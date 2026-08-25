FROM node:20-trixie-slim
USER root
RUN apt-get update \
	&& apt-get install -y --no-install-recommends \
		build-essential \
		make \
		gcc \
		valgrind \
		ca-certificates \
	&& rm -rf /var/lib/apt/lists/*
WORKDIR /var/www/app
COPY app/package.json app/package-lock.json ./
RUN npm ci
COPY bot ./bot
RUN --mount=type=cache,target=./bot/obj/ make -C bot
COPY app ./
