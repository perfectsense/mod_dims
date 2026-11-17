/*
 * DimsDisableEncodedFetch 1 and a source with a space in its name.
 *
 * The server on port 8010 fetches the source as decoded. The fixture
 * "grid copy.png" reaches it two ways: as %20, which the handler decodes to a
 * space the fetch must escape again, and as %2520, which decodes to %20 the
 * fetch must leave alone. Both name the same file at the origin.
 *
 * Copyright 2026 Jeremy Collins
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../lib/common.h"

static const char *
unencoded_url(void)
{
    const char *from_env = getenv("DIMS_TEST_ENCODED_FETCH_OFF_URL");
    return (from_env != NULL && from_env[0] != '\0') ? from_env
                                                     : "http://dims:8010";
}

static dims_response *
unencoded_request(const char *image_url)
{
    char *path = dims_sign_dims4("resize/100x100", image_url, NULL, NULL);
    char full[2048];
    dims_response *response;

    snprintf(full, sizeof(full), "%s%s", unencoded_url(), path);
    response = dims_get_absolute(full);

    free(path);
    return response;
}

static void
check_grid(dims_response *response, const char *what)
{
    dims_image_size size;

    CHECK(response->transport_error == NULL, "the worker must answer: %s",
          response->transport_error ? response->transport_error : "");
    CHECK_INT(response->status, 200, what);
    /* The client default, not DimsNoImageCacheExpire: this is the fixture, not
     * the error image. The fixture sends no cache header of its own. */
    CHECK_STR(dims_header_value(response, "Cache-Control"), "max-age=604800, public",
              "Cache-Control");
    size = dims_must_size(response->body, response->body_len);
    CHECK_INT(size.width, 100, "width");
}

/* A space the handler decoded is escaped again before the fetch. */
static void
test_space_in_source_is_fetched(void)
{
    char *url = dims_fixture_url("grid copy.png");
    dims_response *response = unencoded_request(url);

    check_grid(response, "a source with a space, sent as %20");

    dims_response_free(response);
    free(url);
}

/* A percent escape the handler decoded to %20 is left alone. */
static void
test_percent_escape_in_source_is_kept(void)
{
    char *url = dims_fixture_url("grid%20copy.png");
    dims_response *response = unencoded_request(url);

    check_grid(response, "a source with a space, sent as %2520");

    dims_response_free(response);
    free(url);
}

/* The default server, which encodes the whole URL, keeps fetching the same source. */
static void
test_space_in_source_with_encoded_fetch_on(void)
{
    dims_response *response = dims_request_ops("resize/100x100", "grid copy.png");

    check_grid(response, "a source with a space on the default server");

    dims_response_free(response);
}

const dims_test dims_tests_encoded_fetch_off[] = {
    { "TestSpaceInSourceIsFetched", test_space_in_source_is_fetched, NULL },
    { "TestPercentEscapeInSourceIsKept", test_percent_escape_in_source_is_kept, NULL },
    { "TestSpaceInSourceWithEncodedFetchOn", test_space_in_source_with_encoded_fetch_on, NULL },
    DIMS_TEST_END
};
