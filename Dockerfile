FROM archlinux:latest
USER root
RUN pacman -Sy
RUN pacman -Sy --noconfirm npm gcc make
#RUN pacman -Sy --noconfirm valgrind debuginfod glibc
WORKDIR /var/www/app
COPY app/package.json app/package-lock.json ./
RUN npm ci
COPY bot ./bot
RUN make -C bot
COPY app ./
#ENV DEBUGINFOD_URLS="https://debuginfod.archlinux.org"
