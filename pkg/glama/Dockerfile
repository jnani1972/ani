# Glama MCP directory (glama.ai) check image — NOT required to run the tool.
#
# ani needs NO Docker to run. This image exists only so Glama
# can build a sandbox, launch the stdio MCP server, and run its introspection
# checks, which power the directory's score badge. The same image is exercised
# locally and in CI by pkg/glama/verify.sh to guard against drift.
#
# We pin the exact published release asset and verify its SHA-256 before
# extraction. v0.10.8 publishes the standard (glibc-linked) linux-amd64
# archive; the runtime base is the same debian family, so no static build is
# required. arm64/portable variants will be referenced once a CI release
# publishes them.

FROM debian:bookworm-slim AS fetch
ARG ANI_VERSION=0.10.8
ARG ANI_SHA256=ba8e64111e2cebe15dbe8697d28cdc69abd4098360e1877d4ee447aa912dd034
RUN apt-get update \
 && apt-get install -y --no-install-recommends curl ca-certificates \
 && curl -fsSL "https://github.com/jnani1972/ani/releases/download/v${ANI_VERSION}/ani-linux-amd64.tar.gz" -o /tmp/ani.tar.gz \
 && echo "${ANI_SHA256}  /tmp/ani.tar.gz" | sha256sum -c - \
 && tar -xzf /tmp/ani.tar.gz -C /tmp ani LICENSE THIRD_PARTY_NOTICES.md \
 && chmod +x /tmp/ani \
 && rm /tmp/ani.tar.gz \
 && rm -rf /var/lib/apt/lists/*

FROM debian:bookworm-slim
COPY --from=fetch /tmp/ani /usr/local/bin/ani
COPY --from=fetch /tmp/LICENSE /tmp/THIRD_PARTY_NOTICES.md /usr/share/doc/ani/
ENV ANI_CACHE_DIR=/tmp/ani
ENTRYPOINT ["ani"]
