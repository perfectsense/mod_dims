# Clients

## DimsSigningKey

```apacheconf
DimsSigningKey a-long-random-string
```

The key [`/dims5/`](/endpoints/dims5) signatures are checked against, and the
key an `eurl` is decrypted with. Without it no `/dims5/` request is served.

A leading `sha1:` derives the encryption key the way the older endpoints do,
so one key works across all of them. Without the prefix the key is derived
with HKDF-SHA256, which is the better path.

`/dims5/` has no client id, so this one key covers the endpoint. `/dims3/` and
`/dims4/` keep their per client secrets.

## DimsAddClient

```apacheconf
DimsAddClient <id> <errorImage> <maxAge> <downstreamTtl> <trust> <minSrc> <maxSrc> <secret>
```

Defines a client. Every request has a client id in its path, and a request
whose id is not defined is refused with `500`.

```apacheconf
DimsAddClient development - 604800 604800 trust 300 86400 a-secret
```

| Field | Meaning |
|---|---|
| `id` | the id in the URL |
| `errorImage` | the image sent when a request fails |
| `maxAge` | `Cache-Control: max-age` on the response, in seconds |
| `downstreamTtl` | `Edge-Control: downstream-ttl` on the response, in seconds |
| `trust` | `trust` to use the origin's cache headers, anything else to ignore them |
| `minSrc` | the lowest `max-age` accepted from the origin, in seconds |
| `maxSrc` | the highest `max-age` accepted from the origin, in seconds |
| `secret` | the key `/dims4/` signatures are computed with |

A single dash means the field is unset. A client with no secret refuses every
`/dims4/` request.

Repeat the directive for each client.

## DimsSecretMaxExpiryPeriod

```apacheconf
DimsSecretMaxExpiryPeriod 604800
```

How far ahead an expiry may be, in seconds. A request further ahead than this
is refused, which stops a caller minting URLs that never expire.

On `/dims4/` the expiry is the path segment. `/dims5/` applies this only when
[`DimsDims5Expiry`](#dimsdims5expiry) is on, and then the expiry is the signed
`expires` parameter and a request without one is refused.

A signer that rounds expiry up, as Brightspot's `signedUrlLifetime` does to the
next 24 hours, needs this to be at least its lifetime plus that rounding, or its
own fresh URLs are refused.

`0`, the default, allows any expiry.

## DimsAllowShortHash

```apacheconf
DimsAllowShortHash Off
```

Whether `/dims4/` accepts a short signature hash. `On`, the default, compares
the first six characters of the hash. `Off` requires the full-length hash, so a
hash that is correct in its first six characters and wrong after them is refused.

`/dims5/` always compares the full length, so this directive does not affect it.

## DimsEncryptionAlgorithm

```apacheconf
DimsEncryptionAlgorithm AES/GCM/NoPadding
```

How the `eurl` parameter is encrypted. `AES/ECB/PKCS5Padding` is the default.
`AES/GCM/NoPadding` is the other value, and it is the better one: ECB has no
integrity check and no initialisation vector.

The key is derived from the client's secret: SHA-1 of the secret, the first 16
hexadecimal characters, uppercased.

## DimsDims5Expiry

```apacheconf
DimsDims5Expiry On
```

Whether `/dims5/` honors the expiry settings the server already has. `/dims4/`
always does.

`On` refuses a `/dims5/` request whose signed `expires` parameter, a time in
seconds since the epoch, has passed or is not a time. It applies
[`DimsSecretMaxExpiryPeriod`](#dimssecretmaxexpiryperiod) to `/dims5/`, so with
that above `0` a request needs an `expires`, and it may be at most that many
seconds from now. It also takes the cache headers of a `/dims5/` response from
[`DimsCacheExpire`](/configuration/cache-control#dimscacheexpire).

`Off`, the default, ignores `expires` and both settings, as `/dims5/` always
has.
