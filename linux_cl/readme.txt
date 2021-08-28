Estos scripts sirven para configurar un servicio de linux que se ejectua al arrancar y se repite periódicamente.

El servicio lanza una tarea con curl para realizar una solicitud POST contra el servicio de MonitorCasa, 
tal como hace el cliente de NodeMCU.

La instrucción de CURL está en monitor-casa.sh. Sólo con ejecutar este script de bash se inicia un bucle infinito que lanza la solicitud POST cada 15 minutos.
Este script se ejecuta desde el servicio al arrancar el sistema siguiendo esta configuración.

1. Copiar monitor-casa.sh a una carpeta conocida.
2. Copiar monitor-casa.service a /etc/systemd/system/
3. Editar la ruta de monitor-casa.sh en monitor-casa.service
3. sudo systemctl start monitor-casa
4. sudo systemctl enable monitor-casa
5. sudo update-rc.d monitor-casa default

Para ver el estado del servicio sudo systemctl status monitor-casa

Reiniciar el sistema para comprobar en el log de heroku que están llegando las solicitudes.
