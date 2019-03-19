# Project of INF560: Parallel Computing
Réalisé par Louis Raison et Maël Madon

## Comparer les images (fonctionne mal)
Pour comparer les images produites avec les images traitées de référence, lancer le script compare_ref.sh. Il utilise un algorithme de comparaison octet par octet des fichiers gifs, codé dans src/compare.c.

## Ideas

- Split image according to number of nodes
- cf TD34 INF431
- GIF animes: ok

## TODO

- [x] Modulariser en plusieurs fichiers
- [x] Renommer les fonctions (apply_gray, import, etc.) pour garder la version purement seq.
- [ ] Calculer la taille de l'image et distribuer le travail en fonction.