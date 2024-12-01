#include <iostream>
#include <thread>
#include <vector>
#include <algorithm>
#include <chrono>
#include <mutex>

struct Proceso {
    int id;
    int prioridad; // Menor valor = mayor prioridad
    int tiempo_llegada_ms; // Tiempo de llegada en milisegundos
    std::chrono::time_point<std::chrono::steady_clock> tiempo_inicio;
    std::chrono::time_point<std::chrono::steady_clock> tiempo_fin;

    Proceso(int id, int prioridad, int tiempo_llegada_ms)
        : id(id), prioridad(prioridad), tiempo_llegada_ms(tiempo_llegada_ms) {}
};

std::vector<Proceso> procesos = {
    Proceso(1, 1, 0),    // Alta prioridad, llega en el milisegundo 0
    Proceso(2, 2, 1000), // Prioridad media, llega en el milisegundo 2000
    Proceso(3, 3, 0)  // Baja prioridad, llega en el milisegundo 4000
};

std::mutex recursoA; // Mutex para el recurso A

void ejecutarProceso(Proceso &proceso) {
    // Simular tiempo de llegada
    std::this_thread::sleep_for(std::chrono::milliseconds(proceso.tiempo_llegada_ms));

    // Registrar el tiempo de inicio de espera
    proceso.tiempo_inicio = std::chrono::steady_clock::now();

    while (true) {
        // Intentar bloquear el recurso
        if (recursoA.try_lock()) {
            // Registrar el tiempo en que el proceso accede al recurso
            proceso.tiempo_fin = std::chrono::steady_clock::now();

            std::cout << "Proceso " << proceso.id << " con prioridad " << proceso.prioridad
                      << " accedió al recurso A.\n";

            std::this_thread::sleep_for(std::chrono::seconds(2)); // Simula tiempo de ejecución
            recursoA.unlock(); // Liberar el recurso

            std::cout << "Proceso " << proceso.id << " finalizó y liberó el recurso A.\n";
            break; // Salir del bucle después de completar la ejecución
        } else {
            // El recurso está ocupado, el proceso sigue intentando
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Espera antes de intentar de nuevo
        }
    }
}

int main() {
    // Ordenar los procesos por prioridad (menor valor = mayor prioridad)
    std::sort(procesos.begin(), procesos.end(), [](Proceso &a, Proceso &b) {
        return a.prioridad < b.prioridad;
    });

    // Crear hilos para cada proceso
    std::vector<std::thread> hilos;
    for (auto &proceso : procesos) {
        hilos.emplace_back(ejecutarProceso, std::ref(proceso));
    }

    // Esperar a que todos los hilos terminen
    for (auto &hilo : hilos) {
        hilo.join();
    }

    // Calcular y mostrar el tiempo de espera de cada proceso
    std::cout << "\nTiempos de espera de los procesos:\n";
    for (auto &proceso : procesos) {
        auto tiempo_espera = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 proceso.tiempo_fin - proceso.tiempo_inicio)
                                 .count();
        std::cout << "Proceso " << proceso.id << " esperó " << tiempo_espera << " ms antes de acceder al recurso.\n";
    }

    std::cout << "Todos los procesos han sido ejecutados.\n";
    return 0;
}
