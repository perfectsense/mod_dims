/*
 * A source that downloads but does not decode.
 *
 * The origin serves /corrupt.jpg with a 200 and a body that is not an image.
 * The error image stands in for it, and the response must be cached as a
 * failure: DimsNoImageCacheExpire, 60 in test/conf/dims-test.conf, not the
 * client's own max-age. A success there pins the error image at a CDN for as
 * long as a real image.
 *
 * Copyright 2026 Jeremy Collins
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../lib/common.h"

static void
test_undecodable_source_is_cached_as_a_failure(void)
{
    dims_response *response = dims_request_ops("resize/100x100", "corrupt.jpg");
    dims_image_size size;

    CHECK(response->transport_error == NULL, "the worker must answer: %s",
          response->transport_error ? response->transport_error : "");
    dims_test_logf("an undecodable source returns %ld", response->status);
    CHECK_INT(response->status, 200, "the origin's own status, forwarded");
    CHECK_STR(dims_header_value(response, "Cache-Control"), "max-age=60, public",
              "Cache-Control");

    size = dims_must_size(response->body, response->body_len);
    CHECK_INT(size.width, 100, "the error image, at the requested size");

    dims_response_free(response);
}

/* A crop the error image cannot satisfy still answers as a failure. */
static void
test_undecodable_source_with_a_crop_is_cached_as_a_failure(void)
{
    dims_response *response = dims_request_ops("crop/1000x1000+0+0/resize/100x100",
                                               "corrupt.jpg");

    CHECK(response->transport_error == NULL, "the worker must answer: %s",
          response->transport_error ? response->transport_error : "");
    CHECK_STR(dims_header_value(response, "Cache-Control"), "max-age=60, public",
              "Cache-Control");

    dims_response_free(response);
}

/* The same commands on a source that decodes keep the real image's headers. */
static void
test_decodable_source_keeps_its_own_headers(void)
{
    dims_response *response = dims_request_ops("resize/100x100", "grid.png");

    CHECK_INT(response->status, 200, "the request");
    CHECK_STR(dims_header_value(response, "Cache-Control"), "max-age=3600, public",
              "Cache-Control");

    dims_response_free(response);
}

const dims_test dims_tests_fetch_failure[] = {
    { "TestUndecodableSourceIsCachedAsAFailure",
      test_undecodable_source_is_cached_as_a_failure, NULL },
    { "TestUndecodableSourceWithACropIsCachedAsAFailure",
      test_undecodable_source_with_a_crop_is_cached_as_a_failure, NULL },
    { "TestDecodableSourceKeepsItsOwnHeaders",
      test_decodable_source_keeps_its_own_headers, NULL },
    DIMS_TEST_END
};
