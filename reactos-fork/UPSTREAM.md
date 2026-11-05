# Upstream ReactOS

- Repositorio oficial: https://github.com/reactos/reactos
- Documentación de compilación: https://github.com/reactos/reactos/blob/master/INSTALL
- README oficial: https://github.com/reactos/reactos/blob/master/README.md
- Sitio del proyecto: https://reactos.org/

En esta entrega no se fija un hash de commit porque el entorno no pudo consultar GitHub. Después de clonar, `clone-upstream.sh` guarda el hash real en `UPSTREAM_COMMIT.txt`. No uses `master` como referencia reproducible para una release: fija el hash probado.

## Verificación antes de compilar

`tools/verify-upstream.sh` comprueba que la ruta sea un checkout Git, muestra el commit y avisa si difiere del commit registrado. También detiene el build si hay cambios locales sin commit. Un aviso sobre el remoto no demuestra por sí solo la autenticidad del código: revisa siempre su procedencia.
