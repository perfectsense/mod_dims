/*
 * DimsMaxSourcePixels and DimsMaxSourcePixelsCacheExpire.
 *
 * The limit defaults to no limit, so the main test server cannot exercise it.
 * The server on port 8009 sets it to 200000 pixels, between overlay.png at
 * 200 by 200 and grid.png at 512 by 512, and the cache expire to 3600.
 *
 * Copyright 2026 Jeremy Collins
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../lib/common.h"

static const char *
limited_url(void)
{
    const char *from_env = getenv("DIMS_TEST_PIXEL_LIMITED_URL");
    return (from_env != NULL && from_env[0] != '\0') ? from_env
                                                     : "http://dims:8009";
}

static dims_response *
limited_request(const char *fixture)
{
    char *url = dims_fixture_url(fixture);
    char *path = dims_sign_dims4("resize/100x100", url, NULL, NULL);
    char full[2048];
    dims_response *response;

    snprintf(full, sizeof(full), "%s%s", limited_url(), path);
    response = dims_get_absolute(full);

    free(path);
    free(url);
    return response;
}

/*
 * grid.png holds 262144 pixels. The error image answers at the requested
 * size, with the expire the directive sets rather than DimsNoImageCacheExpire.
 * The error image is grid.png itself, so this also proves the limit does not
 * apply to the error image.
 */
static void
test_source_over_the_pixel_limit_is_refused(void)
{
    dims_response *response = limited_request("grid.png");
    dims_image_size size;

    CHECK(response->transport_error == NULL, "the worker must answer: %s",
          response->transport_error ? response->transport_error : "");
    dims_test_logf("a 262144 pixel source against a 200000 pixel limit returns %ld",
                   response->status);
    CHECK_STR(dims_header_value(response, "Cache-Control"), "max-age=3600, public",
              "Cache-Control");

    size = dims_must_size(response->body, response->body_len);
    CHECK_INT(size.width, 100, "the error image, at the requested size");

    dims_response_free(response);
}

/* overlay.png holds 40000 pixels, so it is served with the client's headers. */
static void
test_source_under_the_pixel_limit_is_served(void)
{
    dims_response *response = limited_request("overlay.png");
    dims_image_size size;

    CHECK_INT(response->status, 200, "a source under the limit");
    CHECK_STR(dims_header_value(response, "Cache-Control"), "max-age=604800, public",
              "Cache-Control");
    size = dims_must_size(response->body, response->body_len);
    CHECK_INT(size.width, 100, "width");

    dims_response_free(response);
}

/*
 * The same source on a server with no limit. This is what the default does,
 * and what every existing deployment keeps doing.
 */
static void
test_no_pixel_limit_accepts_the_same_source(void)
{
    dims_response *response = dims_request_ops("resize/100x100", "grid.png");

    CHECK_INT(response->status, 200, "the request");
    CHECK_STR(dims_header_value(response, "Cache-Control"), "max-age=3600, public",
              "Cache-Control");

    dims_response_free(response);
}

const dims_test dims_tests_pixel_limit[] = {
    { "TestSourceOverThePixelLimitIsRefused", test_source_over_the_pixel_limit_is_refused, NULL },
    { "TestSourceUnderThePixelLimitIsServed", test_source_under_the_pixel_limit_is_served, NULL },
    { "TestNoPixelLimitAcceptsTheSameSource", test_no_pixel_limit_accepts_the_same_source, NULL },
    DIMS_TEST_END
};
