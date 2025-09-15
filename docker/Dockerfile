FROM emscripten/emsdk:3.1.57 AS build

WORKDIR /app
COPY . .

RUN mkdir -p /out web/public \
 && emcc \
      src/gals/*.cpp \
      bridge.cpp \
      -O3 -s MODULARIZE=1 -s EXPORT_NAME=createUniscriptModule -s ENVIRONMENT=web -fwasm-exceptions \
      -s EXPORTED_FUNCTIONS='["_uniscript_compile","_free"]' \
      -s EXPORTED_RUNTIME_METHODS='["cwrap","UTF8ToString"]' \
      -I src \
      -o /out/uniscript.js

# Artifact stage exports the built files at /out
FROM busybox:1.36.1-musl AS artifacts
COPY --from=build /out /out
CMD ["/bin/true"]
