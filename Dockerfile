# Etapa de build
FROM alpine:3.22 AS builder

RUN apk add --no-cache \
    gcc \
    musl-dev \
    libwebsockets-dev \
    openssl-dev

WORKDIR /app

COPY ./src ./src

RUN gcc ./src/_socket.c -o socket -lwebsockets -Os -s

# Etapa final (imagem mínima)
FROM alpine:3.22

RUN apk add --no-cache \
    libwebsockets \
    openssl

WORKDIR /app

COPY --from=builder /app/socket .

CMD ["./socket"]