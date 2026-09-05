FROM archlinux:base-20260830.0.582275

USER root
RUN pacman -Sy
RUN pacman -Sy --noconfirm npm gcc make rsync
ENV ITER=1 
RUN pacman -Sy --noconfirm valgrind debuginfod
RUN pacman -Sy --noconfirm swi-prolog
ENV DEBUGINFOD_URLS="https://debuginfod.archlinux.org"
WORKDIR /var/www/app
COPY app/package.json app/package-lock.json ./
RUN npm ci
COPY bot ./bot
WORKDIR bot/
RUN make -r
WORKDIR ../
COPY app ./
COPY docker-entrypoint.sh /var/
