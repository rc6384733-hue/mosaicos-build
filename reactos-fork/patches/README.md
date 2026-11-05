# Política de parches

No aplicar reemplazos masivos ni cambios automáticos sobre recursos upstream sin revisar su licencia y contexto.

Cada parche debe:
- modificar un objetivo concreto;
- incluir motivo, archivos afectados y prueba reproducible;
- conservar avisos de licencia;
- poder revertirse individualmente;
- compilarse y probarse en QEMU antes de integrarse.

Los primeros parches deben limitarse a identidad visual/metadatos. No tocar kernel, HAL, memoria, almacenamiento ni drivers hasta tener un build upstream reproducible.
