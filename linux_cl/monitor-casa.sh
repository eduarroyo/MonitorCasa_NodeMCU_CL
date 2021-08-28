while true; do curl -X POST -H "Content-Type: application/json" -d '{"monitorId": "paco-arroyo-santaella"}' http://monitor-actividad.herokuapp.com/actualizar; sleep 900; done

