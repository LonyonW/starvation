#include <iostream>
#include <thread>
#include <vector>
#include <algorithm>
#include <chrono>
#include <mutex>
#include <memory>

struct Proceso {
    int id;
    int prioridad; // Menor valor = mayor prioridad
    int tiempo_llegada_ms; // Tiempo de llegada en milisegundos
    std::chrono::time_point<std::chrono::steady_clock> tiempo_inicio;
    std::chrono::time_point<std::chrono::steady_clock> tiempo_fin;
    bool prioridad_mejorada; // Para indicar si la prioridad fue mejorada

    Proceso(int id, int prioridad, int tiempo_llegada_ms)
        : id(id), prioridad(prioridad), tiempo_llegada_ms(tiempo_llegada_ms), prioridad_mejorada(false) {}
};

std::vector<std::shared_ptr<Proceso>> procesos = {
    std::make_shared<Proceso>(1, 1, 0),    // Alta prioridad, llega en el milisegundo 0
    std::make_shared<Proceso>(2, 2, 1000), // Prioridad media, llega en el milisegundo 1000
    std::make_shared<Proceso>(3, 3, 0)     // Prioridad baja, llega en el milisegundo 0
};

std::mutex recursoA; // Mutex para el recurso A
std::mutex procesos_mutex; // Mutex para proteger la modificación de prioridades

void mejorarPrioridad() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Revisar cada 100 ms

        std::lock_guard<std::mutex> lock(procesos_mutex);

        for (auto &proceso : procesos) {
            // Mejorar prioridad solo si el proceso tiene prioridad > 1 y está esperando
            if (!proceso->prioridad_mejorada && proceso->tiempo_inicio.time_since_epoch().count() > 0 &&
                proceso->tiempo_fin.time_since_epoch().count() == 0 && proceso->prioridad > 1) {
                auto tiempo_espera = std::chrono::duration_cast<std::chrono::milliseconds>(
                                         std::chrono::steady_clock::now() - proceso->tiempo_inicio)
                                         .count();

                if (tiempo_espera > 1500) { // Se mejora prioridad si espera más de 1500 ms
                    std::cout << "Mejorando prioridad del Proceso " << proceso->id << " por esperar más de 1500 ms.\n";
                    proceso->prioridad = 1; // Mejora la prioridad al máximo
                    proceso->prioridad_mejorada = true;
                }
            }
        }

        // Salir si todos los procesos han sido ejecutados
        if (std::all_of(procesos.begin(), procesos.end(), [](std::shared_ptr<Proceso> &p) {
                return p->tiempo_fin.time_since_epoch().count() > 0;
            })) {
            break;
        }
    }
}

void ejecutarProceso(std::shared_ptr<Proceso> proceso) {
    // Simular tiempo de llegada
    std::this_thread::sleep_for(std::chrono::milliseconds(proceso->tiempo_llegada_ms));

    // Registrar el tiempo de inicio de espera
    proceso->tiempo_inicio = std::chrono::steady_clock::now();

    while (true) {
        {
            std::lock_guard<std::mutex> lock(procesos_mutex);

            // Ordenar procesos por prioridad antes de cada intento de ejecución
            std::sort(procesos.begin(), procesos.end(), [](std::shared_ptr<Proceso> &a, std::shared_ptr<Proceso> &b) {
                return a->prioridad < b->prioridad;
            });
        }

        // Intentar bloquear el recurso
        if (recursoA.try_lock()) {
            // Registrar el tiempo en que el proceso accede al recurso
            proceso->tiempo_fin = std::chrono::steady_clock::now();

            std::cout << "Proceso " << proceso->id << " con prioridad " << proceso->prioridad
                      << " accedió al recurso A.\n";

            std::this_thread::sleep_for(std::chrono::seconds(2)); // Simula tiempo de ejecución
            recursoA.unlock(); // Liberar el recurso

            std::cout << "Proceso " << proceso->id << " finalizó y liberó el recurso A.\n";
            break; // Salir del bucle después de completar la ejecución
        } else {
            // El recurso está ocupado, el proceso sigue intentando
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Espera antes de intentar de nuevo
        }
    }
}

int main() {
    // Crear un hilo para monitorear y mejorar las prioridades
    std::thread monitor_prioridad(mejorarPrioridad);

    // Crear hilos para cada proceso
    std::vector<std::thread> hilos;
    for (auto &proceso : procesos) {
        hilos.emplace_back(ejecutarProceso, proceso);
    }

    // Esperar a que todos los hilos terminen
    for (auto &hilo : hilos) {
        hilo.join();
    }

    // Esperar a que el monitor de prioridad termine
    monitor_prioridad.join();

    // Calcular y mostrar el tiempo de espera de cada proceso
    std::cout << "\nTiempos de espera de los procesos:\n";
    for (auto &proceso : procesos) {
        auto tiempo_espera = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 proceso->tiempo_fin - proceso->tiempo_inicio)
                                 .count();
        std::cout << "Proceso " << proceso->id << " esperó " << tiempo_espera << " ms antes de acceder al recurso.\n";
    }

    std::cout << "Todos los procesos han sido ejecutados.\n";
    return 0;
}
