
set -e

# Le moteur est déjà compilé dans l'image ; on le recompile ici parce que
# `develop.watch` de compose synchronise bot/ à chaud, et que les sources
# montées peuvent être plus récentes que le binaire. route.ts sait aussi le
# refaire à la demande, mais le faire ici évite d'en payer le coût sur la
# première requête.
( cd bot && sh build.sh )
npm run dev
