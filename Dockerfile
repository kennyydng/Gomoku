FROM archlinux:base-20260830.0.582275

USER root
RUN pacman -Sy
RUN pacman -Sy --noconfirm npm gcc rsync
ENV ITER=1 
RUN pacman -Sy --noconfirm valgrind debuginfod
ENV DEBUGINFOD_URLS="https://debuginfod.archlinux.org"
WORKDIR /var/www/app
COPY app/package.json app/package-lock.json ./
RUN npm ci
COPY bot ./bot
WORKDIR bot/
# Le binaire que l'API web lance réellement (bot/Gomoku). `make` produisait un
# binaire par combinaison de règles, héritage de l'époque où elles étaient
# figées à la compilation ; elles sont désormais lues sur stdin, donc un seul
# binaire suffit — et le compiler ici évite de payer la compilation à la
# première requête.
RUN sh build.sh
WORKDIR ../
COPY app ./
COPY docker-entrypoint.sh /var/
