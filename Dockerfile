FROM archlinux:base-20260830.0.582275

USER root
# --disable-sandbox : pacman tente de larguer ses privileges via seccomp pour
# telecharger et extraire. Sous emulation qemu (hote arm64), l'appel echoue —
# « error restricting syscalls via seccomp: 22 » — et le build s'arrete des la
# premiere synchronisation. Le bac a sable perd peu ici : le build tourne deja
# en root, dans un conteneur jetable, depuis une image epinglee et en HTTPS.
# Sur un hote amd64 le drapeau ne change rien au resultat.
RUN pacman -Sy --disable-sandbox
RUN pacman -Sy --noconfirm --disable-sandbox npm gcc make rsync
ENV ITER=1 
RUN pacman -Sy --noconfirm --disable-sandbox valgrind debuginfod
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
