# Etapa de build
FROM alpine:3.22 AS builder

RUN apk add --no-cache \
    gcc \
    musl-dev \
    libwebsockets-dev \
    openssl-dev \
    cjson-dev

WORKDIR /app

COPY ./src ./src


RUN gcc ./src/_socket.c -I./libs -I/usr/include/cjson -o socket -lwebsockets -lcjson -Os -s

# Etapa final (imagem mínima)
FROM alpine:3.22

RUN apk add --no-cache \
    libwebsockets \
    openssl \
    cjson

WORKDIR /app

COPY --from=builder /app/socket .

CMD ["./socket"]