# Informe de implementación y verificación: fases 0 a 4

**Proyecto:** Sistemas Operativos, pipes sobre Selfie.  
**Fecha:** 6 de noviembre de 2026, Perú.  
**Resultado:** los 29 requisitos principales de las fases0-4 del plan están implementados y sus gates aprobados en la configuración verificada.

## 1. Alcance y resultado entregado

Se partió del `selfie.c` de `Pipes.zip` y se produjo un archivo final compilable por GCC y por starc. Se mantuvieron los tests originales de fases0-4; no se cambiaron sus expectativas para conseguir resultados favorables. Se añadieron pruebas de frontera con retorno de error real si una comprobación falla.

Esta entrega implementa procesos y transferencia de bytes **sin bloqueo de pipes**. Los estados BLOCKED y el campo next_waiter están reservados como exige la fase1; no se encolan durmientes, no se detectan deadlocks ni se implementan los wakeups de fases5-6.

## 2. Cambios realizados por fase

### Fase 0: salida confiable

Se repone el terminador inmediatamente después de `copy_buffer` dentro de la rama de éxito. La copia por palabras deja de ocasionar impresión de bytes fuera del tamaño solicitado.

Además, se reemplaza el uso de `printf("%s")` para stdout anfitrión por emisión de exactamente los caracteres pedidos con `printf("%c")`. Esto mantiene el orden de stdio y permite incluir NUL en la salida. Un error de salida conserva -1 cuando no se emitió ningún byte y una cantidad parcial cuando ya hubo progreso. El resultado se reduce según el ABI de syscalls.

Se trata size0 antes de llamar a `copy_buffer`, porque esa función interpreta tamaño0 como copia de cadena, no como operación vacía. Las escrituras de tamaño positivo con memoria inválida siguen pasando por el camino de error.

**Evidencia:** p0 muestra exactamente `abc|0123456789|X`, con newline y exit0. El caso adicional de consola contrasta los cuatro bytes `41 00 42 FF` y su retorno4. El original anterior al cambio reproducía el texto excedente; los resultados negativos están preservados en evidence cuando son pertinentes.

### Fase 1: contexto y planificador

Se amplía CONTEXTENTRIES de32 a34 y se documentan los offsets32 y33. Se incorporan constantes READY=0, BLOCKED=1 y EXITED=2, accesores e inicialización explícita al crear o reciclar un contexto.

`schedule(from)` recorre circularmente la lista desde el siguiente contexto y considera `from` al final. Solo selecciona READY pertenecientes a esta instancia del kernel (`parent == MY_CONTEXT`). Los contextos virtuales cacheados de otro nivel se despachan mediante el mecanismo existente de su padre, que permanece intacto.

`mipster` guarda el tipo de excepción antes de manejarla, marca EXITED sin terminar prematuramente y selecciona otro contexto al finalizar o vencer el timer. Cuando no quedan contextos elegibles, devuelve el código del último que terminó. Se conserva el comportamiento de terminación del único proceso.

**Corrección adicional importante:** se conserva el presupuesto restante del timer al retornar de una syscall o page fault. Solo se concede un turno nuevo al cambiar de proceso, vencer el timer o restablecer uno desactivado. Sin esto, un proceso que consulta repetidamente un pipe vacío puede renovar su turno indefinidamente. El test con tres hijos detectó ese problema y pasa después de corregirlo.

**Evidencia:** p1 conserva el mensaje y exit7; p2 demuestra alternancia, aislamiento y supervivencia del hijo a la salida del padre. El test adicional de IPC progresa con polling frecuente y varios hijos; el harness comprueba reinicialización de campos al reciclar contextos.

### Fase 2: fork y memoria privada

Se añade SYSCALL_FORK=220, su emisor de cero argumentos, registro en el bootstrap, contabilidad de registros de la instrucción y despacho al manejador.

`syscall_return` centraliza a0 y el avance del PC. `implement_fork` crea un contexto con padre de kernel, lo mantiene no elegible mientras se inicializa y copia nombre, segmentos, break y los32 registros. El break se copia antes de mapear para preservar la clasificación de páginas bajas/altas.

Se recorre el intervalo bajo y el alto del padre. Cada página efectivamente mapeada se copia palabra a palabra a un frame nuevo; los huecos no mapeados se mantienen como huecos. El código también tiene frames privados. El hijo continúa después de ecall con a0=0 y el padre con un identificador creciente. El valor reservado 4294967295 no se entrega como PID.

Cuando la comprobación de disponibilidad detecta que no puede continuar la copia, el hijo queda EXITED y el padre recibe error. Los frames ya asignados no se recuperan por la limitación del asignador original. Una asignación interna de tabla o de memoria anfitriona todavía puede terminar la máquina: no se presenta esa política como rollback total ante OOM.

**Evidencia:** p2_fork da las ocho comprobaciones en el orden del plan; p2_exit termina con5 después de dejar vivir al hijo. El caso adicional comprueba PIDs crecientes, tres hijos, privacidad de variables globales/stack/heap y páginas distantes con un hueco inicialmente no tocado.

### Fase 3: registro y ciclo de vida

Se añaden SYSCALL_PIPE_CREATE=500 y DESTROY=503, emisores, bootstrap, contabilidad y despacho. La tabla se reserva a demanda con16 entradas; cero no es ID válido y hay15 slots utilizables.

Cada pipe usa nueve palabras inicializadas a cero: ID, buffer, r, w, n, dos punteros de lectores y dos de escritores. El almacenamiento circular es de16 bytes. Se incorporan accesores de los campos de datos y validación centralizada de IDs antes de indexar.

Create elige el menor ID libre; destroy invalida el slot y devuelve0. El comentario TODO documenta que la memoria del objeto/buffer no se libera. En fase4 no existen procesos esperando sobre un pipe, por lo que no hay wakeups pendientes.

Se reinicia la tabla y el contador de PID en `reset_microkernel`, evitando referencias a contextos reciclados de una ejecución anterior. La CLI heredada retorna tras su primer `-m`, por lo que este reinicio se verifica con un harness nativo que llama al kernel dos veces dentro del mismo proceso anfitrión, no con una cadena de `-m` que la CLI no llega a ejecutar.

**Evidencia:** ocho comprobaciones originales de registro, IDs extremos y slots recreados en pruebas adicionales; harness de reinicio aprobado.

### Fase 4: camino de datos sin bloqueo

Se añaden SYSCALL_PIPE_WRITE=501 y READ=502, ambos emisores de tres argumentos, registro, contabilidad y despacho. `guest_word` valida tanto la dirección exacta del byte como la palabra que lo contiene, rechaza segmentos no permitidos y traduce a memoria física. Las páginas de heap reservadas pero no tocadas se mapean bajo demanda y actualizan el contador de heap mapeado.

El manejador compartido de transferencia mueve un byte a la vez mediante `load_character` y `store_character`. Mantiene `0 <= n <=16`, índices dentro de0..15 y `w=(r+n)%16`. Valida antes de cambiar el estado del ring; evita overflow de la suma de dirección y desplazamiento. No usa `copy_buffer` para datos del pipe.

ID inválido tiene prioridad sobre longitud0. Con ID válido y longitud0 no se consulta memoria. Vacío/lleno retorna la cantidad acumulada, incluso0, antes de intentar copiar un byte. Una dirección inválida al primer byte devuelve error y no modifica el ring; si aparece después de k bytes, retorna k. Todo camino completado avanza exactamente una instrucción y entrega el resultado mediante el ABI.

**Corrección adicional de bytes nativos:** el código base realiza acceso por palabras porque C* carece de acceso a bytes. En GCC, char es un byte y puede ser signed. Se ajustaron `load_character` y `store_character` para usar acceso directo cuando `sizeof(char)==1`; en C* se conserva el acceso por palabras y se normalizan los valores a8 bits antes de restar o insertar. Esto evita tocar fuera de un literal corto nativo y preserva los bytes vecinos al transferir128..255.

El experimento `binary_before_signedness_fix.log` conserva un FAIL anterior a esta corrección. Es evidencia negativa deliberada, no un fallo de la versión final. Los logs finales correspondientes están en `evidence/extra/` y muestran todas las comprobaciones aprobadas.

**Evidencia:** p4_transfer da33 OK y p4_temporary tres OK. Los casos añadidos prueban bytes binarios, memoria desalineada, límites de página, transferencia parcial por dirección inválida y64 rondas de buffer circular con datos retenidos antes de insertar más bytes.

## 3. Trazabilidad de requisitos

| Requisitos | Implementación / evidencia |
|---|---|
| R0.1-R0.2 | Terminador posterior a la copia; ruta de error para tamaño positivo; stdout por tamaño. p0 y consola binaria. |
| R0.3 | Construcción, autocompilación y anidación de phase0. |
| R1.1-R1.3 | Layout34, accesores y reinicialización; harness de reciclado. |
| R1.4-R1.5 | Round robin, excepción conservada, estado EXITED, último resultado y rama de padre intacta. p2 y tres hijos. |
| R1.6 | p0/p1 y gates con un contexto. |
| R2.1-R2.3 | ID220, emisor/bootstrap y retorno común; autocompilación y p2. |
| R2.4-R2.6 | Segmentos y break antes de mapear;32 registros; frames privados; PC y retornos. p2 e IPC adicional. |
| R2.7 | Despacho fork, ejercitado nativamente y dentro de Selfie. |
| R3.1-R3.4 | IDs/emisores, objeto9, capacidad16, tabla16 y lookup seguro. Registro y errores extremos. |
| R3.5-R3.6 | Menor slot, lleno, destroy, reutilización y TODO de memoria. p3/edges. |
| R3.7 | Código de kernel antes del dispatcher e integración de ambos manejadores. Gates. |
| R4.1-R4.3 | Emisores3 argumentos; traducción/páginas; copia por byte/ring. p4 y casos de memoria/binarios. |
| R4.4 | Cantidad parcial,0, IDs y errores antes/después de progreso. edges/boundaries. |
| R4.5 | Vacío/lleno retorna0. p4_temporary y casos adicionales. |
| R4.6 | ID y longitud0 antes de disponibilidad. edges. |

## 4. Validación reproducible

| Grupo | Número de controles aprobados | Alcance |
|---|---:|---|
| Gates por fase | 34 | 5 construcciones,5 autocompilaciones,5 anidaciones con hello y19 ejecuciones acumulativas de pruebas originales. |
| Adicionales | 16 | 7 pruebas C* nativas,1 harness de reinicio,7 pruebas C* anidadas y1 igualdad de fuente/checkpoint. |
| ASan/UBSan | 8 | fork, transferencia y nuevos recorridos de memoria con sanitizadores. |
| **Total** | **58** | Resultado de los verificadores entregados. |

El total cuenta ejecuciones/comprobaciones de los verificadores, no58 casos C* distintos. Dentro de cada ejecución hay comprobaciones adicionales: por ejemplo, las33 de p4_transfer.

### Resultados originales de la fase final

| Prueba | Salida o comprobaciones | Exit |
|---|---|---:|
| p0_write | `abc\|0123456789\|X` y newline | 0 |
| p1_exit | `survived timer interrupts` y newline | 7 |
| p2_fork | 04,05,01,02,03,06,07,08 | 0 |
| p2_exit | 01,02 | 5 |
| p3_registry | 01..08 | 0 |
| p4_transfer | 01..33 | 0 |
| p4_temporary | 01..03 | 0 |

Los verificadores comparan el bloque de salida del programa invitado de forma exacta, junto con el retorno y ausencia de FAIL/diagnósticos. En ejecución anidada comprueban también ambas líneas de terminación. No dependen del código de retorno de `sed` ni confunden un programa que devuelve7 o5 correctamente con un fallo.

La construcción normal sigue el comando del plan con `-w`; esto no implica ausencia de advertencias nativas bajo todas las opciones de GCC. El requisito de cero errores y warnings de autocompilación se verifica explícitamente con starc.

## 5. Entrega y límites

El archivo a usar es `selfie.c`; los checkpoints se incluyen para revisar la progresión y reproducir cada gate. `cambios_fases_0_4.patch` permite inspeccionar la diferencia completa frente al original recibido. Los documentos en reference se conservaron como fuentes históricas y no se reescribieron para fingir que la fase5/6 está implementada.

La versión final se probó con Linux64, RISC-U64 y mipster. No se extrapola a todos los motores, combinaciones de recolección de basura, configuraciones32 bits o cualquier carga posible. La reclamación del almacenamiento de pipes y la recuperación integral ante agotamiento de recursos siguen las limitaciones indicadas en README y en los TODO de kernel.

Para avanzar a fase5 se deben sustituir únicamente las ramas de cero progreso por encolado y bloqueo, preservar argumentos/PC para reintentar y actualizar las expectativas de p4_temporary. Antes de utilizar la prueba futura p5_stream hay que corregir su longitud58 a57. La ambigüedad de reutilización de IDs con procesos durmientes pertenece a fase6; no se manifiesta en esta versión porque ninguna operación de pipe duerme.

Los controles ejecutados acreditan el cierre de las fases solicitadas dentro del contrato y configuración descritos; no constituyen una prueba formal de ausencia universal de errores.
