# Pipes en Selfie: implementación completa hasta la fase 4

Entrega del 6 de noviembre de 2026. El código principal es **`selfie.c`**.

Se implementaron las fases **0, 1, 2, 3 y 4** del plan recibido. La fase 4 termina deliberadamente en una API **sin bloqueo**: leer un pipe vacío o escribir uno lleno devuelve 0 inmediatamente. Las fases 5 y 6 son trabajo posterior.

## Compilar y ejecutar

Entorno verificado: Linux x86_64, GCC 13.3.0 y Python 3.12. No utiliza paquetes Python externos. Se requiere una terminal con GCC y Python; Make es opcional.

Desde esta carpeta:

```bash
make
./selfie -c tests/p4_transfer.c -m 2
./selfie -c tests/p4_temporary.c -m 2
```

Si no usas Make, la compilación equivalente es:

```bash
gcc -w -O2 -m64 -D'uint64_t=unsigned long' selfie.c -o selfie
```

Las pruebas C* se compilan con `./selfie -c ...`; no se compilan directamente con GCC. La excepción es `tests/x_reset_kernel.c`, un arnés nativo que ejecuta automáticamente el verificador adicional.

## Reproducir la verificación

```bash
make verify
```

O, sin Make:

```bash
python3 verify.py
python3 verify_extra.py
```

El primer script construye cada checkpoint y ejecuta los gates de cada fase, incluyendo todas las pruebas anteriores. El segundo verifica los casos adicionales y la ejecución anidada. Ambos devuelven un código distinto de cero si hay errores, advertencias de starc, salida diferente de la esperada, comprobaciones faltantes o FAIL.

Para los controles adicionales de memoria nativa:

```bash
make sanitize
```

O:

```bash
python3 verify_sanitizers.py
```

Este control requiere GCC con soporte para AddressSanitizer y UndefinedBehaviorSanitizer. Se desactiva la detección de fugas porque Selfie conserva asignaciones de su arena y el plan acepta que destroy no libere físicamente los objetos; sí se comprueban accesos y comportamiento indefinido en los recorridos ejecutados.

Los programas `p1_exit` y `p2_exit` terminan deliberadamente con códigos 7 y 5. Esos retornos son éxitos según el contrato de sus pruebas.

## Qué está implementado

| Fase | Comportamiento implementado |
|---|---|
| 0 | `write` respeta el tamaño solicitado; salida de consola con bytes nulos; longitud cero sin examinar el buffer. |
| 1 | Contextos de 34 campos, READY/BLOCKED/EXITED, enlace de espera reservado y scheduler round robin. Una syscall no renueva el turno de CPU. |
| 2 | `fork`, copia privada de páginas/registros y retorno distinto para padre/hijo. La máquina sigue ejecutando los contextos restantes. |
| 3 | Registro global de 15 IDs utilizables, creación, validación, destrucción y reutilización del menor ID libre. |
| 4 | Lectura/escritura FIFO de bytes; capacidad16; alineación arbitraria; transferencias parciales; páginas bajo demanda; errores de dirección; vacío/lleno retornan0. |

## API y reglas importantes

```c
uint64_t fork();
uint64_t pipe_create();
uint64_t pipe_write(uint64_t id, uint64_t* buffer, uint64_t size);
uint64_t pipe_read(uint64_t id, uint64_t* buffer, uint64_t size);
uint64_t pipe_destroy(uint64_t id);
```

Estas firmas describen la biblioteca que genera starc; los emisores incorporados hacen que los programas C* puedan llamar a las operaciones sin incluir headers. Los valores de retorno son:

- `fork`: 0 en hijo, PID positivo creciente en padre; error representado como 4294967295 cuando el manejador puede devolverlo.
- `pipe_create`: menor ID libre de1 a15; 4294967295 si no hay slots.
- `pipe_destroy`: 0 o 4294967295 si el ID no existe.
- Read/write: número de bytes transferidos. Puede ser menor que size.
- Read/write sin disponibilidad: 0, sin bloquear.
- ID inválido: 4294967295, incluso si size=0.
- ID válido y size=0: 0, sin validar el buffer ni esperar disponibilidad.
- Dirección inválida antes del primer byte que se intenta mover: error y pipe sin cambios.
- Dirección inválida después de k bytes: devuelve k y conserva solo esos efectos.

Si no hay disponibilidad, se devuelve0 antes de examinar direcciones: no se intenta copiar ningún byte. Los IDs son globales dentro de una ejecución y se comparten entre procesos; los espacios de memoria de usuario siguen siendo privados.

No se implementan descriptores de extremos Unix, EOF por cierre, SIGPIPE o EPIPE. No se añade una promesa de atomicidad POSIX para mensajes. Estas son decisiones del plan académico, que usa una API propia de IDs.

## Resultados incluidos

Se incluyen **58 controles aprobados**:

- 34 controles acumulativos entre los cinco gates.
- 16 controles adicionales, incluidos siete casos anidados, reinicio del kernel y correspondencia del checkpoint final.
- 8 ejecuciones con sanitizadores.

Las siete pruebas originales hasta fase4 pasan. `p4_transfer` da sus 33 comprobaciones y `p4_temporary` sus tres comprobaciones, con exit0.

Los controles añadidos verifican bytes0/128/255 y preservación de vecinos, cruce de página, error parcial al alcanzar el final del heap, IDs extremos, longitud cero, 64 ciclos de buffer circular y comunicación de tres hijos con aislamiento de páginas mapeadas y no tocadas.

## Contenido

| Ruta | Uso |
|---|---|
| `selfie.c` | Implementación final para compilar y entregar. |
| `tests/` | Siete pruebas originales de fases0-4 y ocho archivos adicionales; uno es el arnés nativo. |
| `verify.py` | Gates de fases0-4 y regresiones acumuladas. |
| `verify_extra.py` | Casos adicionales, arnés nativo y ejecución anidada. |
| `verify_sanitizers.py` | Ocho recorridos con ASan/UBSan. |
| `checkpoints/phase0.c` a `phase4.c` | Código completo y verificable al cierre de cada fase. |
| `evidence/` | Logs, resultados JSON y hashes de esta entrega. |
| `INFORME_IMPLEMENTACION.md` | Explicación de decisiones, requisitos y límites. |
| `cambios_fases_0_4.patch` | Diferencia entre el `selfie.c` recibido y el final. |
| `reference/` | Plan y guías originales, conservados como referencia histórica. |
| `reference/future_tests/` | Pruebas originales de fases5-6, fuera de la suite de esta entrega. |

`checkpoints/phase4.c` y `selfie.c` son idénticos. El ejecutable se genera con `make`; no se exige reutilizar un binario de otra máquina.

## Límites documentados

La configuración verificada es RISC-U de64 bits sobre mipster. No se certifican los demás motores ni la combinación con GC. El plan mantiene la reclamación de memoria del kernel como TODO; destruir libera un ID, no la asignación física. El asignador base todavía puede terminar la máquina con21 al agotar recursos internos; no se promete recuperación completa de cualquier OOM.

Las guías originales no son la especificación del comportamiento final: el HTML ampliado muestra una escritura que espera completar todos sus bytes; esta entrega sigue el contrato parcial de fase4. La prueba futura `p5_stream` conserva su contenido original en reference y debe corregirse de58 a57 bytes antes de utilizarse en la fase5.
