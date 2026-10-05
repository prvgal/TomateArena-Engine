/*
 * tomate_api.cpp
 * Copyright (C) 2026 prvgal [Pablo Rivero Galvín] <rivdevapps@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty统; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "../include/Juego.hpp"
#include "../include/JuegoTutorial.hpp"
#include "../include/IA.hpp"
#include <cstdio>
#include <exception>
#include <new>
#include <string>

#if defined(__ANDROID__)
    #include <android/log.h>
    #define TOMATE_LOG(fn, msg) __android_log_print(ANDROID_LOG_ERROR, "tomate_engine", "%s: %s", fn, msg)
#else
    #define TOMATE_LOG(fn, msg) std::fprintf(stderr, "[tomate_engine] %s: %s\n", fn, msg)
#endif

#if defined(_WIN32)
    #define FFI_EXPORT __declspec(dllexport)
#else
    #define FFI_EXPORT __attribute__((visibility("default"))) __attribute__((used))
#endif

struct FfiDecisionAccion {
    int accion; // 0: DESAGRADO_DESCARTAR, 1: USAR_EFECTO, 2: INTERCAMBIAR
    size_t posManoIntercambio;
};

struct FfiDecisionIntercambioRival {
    size_t posPropia;
    size_t idxRival;
    size_t posRival;
};

// Ninguna excepción C++ puede cruzar la frontera extern "C" hacia Dart:
// provocaría std::terminate y cerraría la app. Todo pasa por estos helpers.
namespace {

template <typename R, typename F>
R seguro(const char* fn, R porDefecto, F&& f) noexcept {
    try {
        return f();
    } catch (const std::exception& e) {
        TOMATE_LOG(fn, e.what());
    } catch (...) {
        TOMATE_LOG(fn, "excepción desconocida");
    }
    return porDefecto;
}

template <typename F>
void seguroVoid(const char* fn, F&& f) noexcept {
    try {
        f();
    } catch (const std::exception& e) {
        TOMATE_LOG(fn, e.what());
    } catch (...) {
        TOMATE_LOG(fn, "excepción desconocida");
    }
}

inline bool neg(int v) { return v < 0; }

}

extern "C" {

    // ---------------- CICLO DE VIDA ----------------

    FFI_EXPORT Juego* crear_juego_tutorial() {
        return seguro<Juego*>("crear_juego_tutorial", nullptr, [] { return static_cast<Juego*>(new JuegoTutorial()); });
    }

    FFI_EXPORT Juego* crear_juego() {
        return seguro<Juego*>("crear_juego", nullptr, [] { return new Juego(); });
    }

    FFI_EXPORT void destruir_juego(Juego* juego) {
        seguroVoid("destruir_juego", [&] { delete juego; });
    }

    // Devuelve false si falla (ya no se relanza la excepción).
    FFI_EXPORT bool iniciar_juego(Juego* juego, int numJugadores) {
        if (!juego || numJugadores < 2) return false;
        return seguro<bool>("iniciar_juego", false, [&] {
            juego->iniciar(static_cast<size_t>(numJugadores));
            return true;
        });
    }

    // ---------------- CONSULTAS ----------------

    FFI_EXPORT int obtener_jugador_actual(Juego* juego) {
        if (!juego) return 0;
        return seguro<int>("obtener_jugador_actual", 0, [&] { return static_cast<int>(juego->jugadorActual()); });
    }

    FFI_EXPORT bool es_fin_de_partida(Juego* juego) {
        if (!juego) return true;
        return seguro<bool>("es_fin_de_partida", true, [&] { return juego->esFin(); });
    }

    FFI_EXPORT bool hubo_rebaraje_ultimo_robo(Juego* juego) {
        if (!juego) return false;
        return seguro<bool>("hubo_rebaraje_ultimo_robo", false, [&] { return juego->huboRebarajeUltimoRobo(); });
    }

    FFI_EXPORT int obtener_carta_descarte(Juego* juego) {
        if (!juego) return -1;
        return seguro<int>("obtener_carta_descarte", -1, [&] {
            auto opt = juego->cartaDescarte();
            if (!opt) return -1;
            if (opt->tipo == TipoCartaRobada::JOKER) return 100;
            if (opt->tipo == TipoCartaRobada::DESCARTE) return 101;
            if (opt->carta) return static_cast<int>(*opt->carta);
            return -1;
        });
    }

    FFI_EXPORT int obtener_carta_robada(Juego* juego) {
        if (!juego) return -1;
        return seguro<int>("obtener_carta_robada", -1, [&] {
            if (juego->cartaRobadaEsJoker()) return 100;
            if (juego->cartaRobadaEsDescarte()) return 101;
            auto opt = juego->valorCartaRobada();
            return opt ? static_cast<int>(*opt) : -1;
        });
    }

    FFI_EXPORT int obtener_numero_cartas_jugador(Juego* juego, int idJugador) {
        if (!juego || neg(idJugador)) return 0;
        return seguro<int>("obtener_numero_cartas_jugador", 0, [&] {
            return static_cast<int>(juego->jugador(idJugador).numeroCartasActuales());
        });
    }

    FFI_EXPORT int obtener_puntuacion_jugador(Juego* juego, int idJugador) {
        if (!juego || neg(idJugador)) return 0;
        return seguro<int>("obtener_puntuacion_jugador", 0, [&] { return juego->jugador(idJugador).puntuacion(); });
    }

    FFI_EXPORT bool es_cierre(Juego* juego) {
        if (!juego) return false;
        return seguro<bool>("es_cierre", false, [&] { return juego->cierre(); });
    }

    FFI_EXPORT bool descarte_es_reaccionable(Juego* juego) {
        if (!juego) return false;
        return seguro<bool>("descarte_es_reaccionable", false, [&] { return juego->descarteEsReaccionable(); });
    }

    FFI_EXPORT bool hubo_penalizacion_ultimo_intento(Juego* juego) {
        if (!juego) return false;
        return seguro<bool>("hubo_penalizacion_ultimo_intento", false, [&] { return juego->huboPenalizacionUltimoIntento(); });
    }

    FFI_EXPORT int obtener_ganador(Juego* juego, bool soloHumanoDesempata) {
        if (!juego) return -1;
        return seguro<int>("obtener_ganador", -1, [&] {
            auto opt = juego->obtenerGanador(soloHumanoDesempata);
            return opt ? static_cast<int>(*opt) : -1;
        });
    }

    FFI_EXPORT int obtener_ultimo_criterio_desempate(Juego* juego) {
        if (!juego) return 0;
        return seguro<int>("obtener_ultimo_criterio_desempate", 0, [&] { return juego->ultimoCriterioDesempate(); });
    }

    // ---------------- ACCIONES ----------------

    FFI_EXPORT bool robar_del_monton(Juego* juego) {
        if (!juego) return false;
        return seguro<bool>("robar_del_monton", false, [&] { return juego->robarDelMonton(); });
    }

    FFI_EXPORT void descartar_carta_robada(Juego* juego) {
        if (!juego) return;
        seguroVoid("descartar_carta_robada", [&] { juego->descartarCartaRobada(); });
    }

    FFI_EXPORT bool intercambiar_carta_robada(Juego* juego, int posicionMano) {
        if (!juego || neg(posicionMano)) return false;
        return seguro<bool>("intercambiar_carta_robada", false, [&] {
            return juego->intercambiarCartaRobada(static_cast<size_t>(posicionMano));
        });
    }

    FFI_EXPORT void declarar_cierre(Juego* juego, int idJugador) {
        if (!juego || neg(idJugador)) return;
        seguroVoid("declarar_cierre", [&] { juego->cerrarPartida(idJugador); });
    }

    FFI_EXPORT void terminar_fase_revelado(Juego* juego) {
        if (!juego) return;
        seguroVoid("terminar_fase_revelado", [&] { juego->terminarFaseRevelado(); });
    }

    FFI_EXPORT int consultar_carta_mano(Juego* juego, int idJugador, int indice) {
        if (!juego || neg(idJugador) || neg(indice)) return -1;
        return seguro<int>("consultar_carta_mano", -1, [&] {
            auto opt = juego->consultarCartaMano(static_cast<size_t>(idJugador), static_cast<size_t>(indice));
            return opt ? static_cast<int>(*opt) : -1;
        });
    }

    FFI_EXPORT bool intentar_descarte_fuera_de_turno(Juego* juego, int idJugador, int posMano) {
        if (!juego || neg(idJugador) || neg(posMano)) return false;
        return seguro<bool>("intentar_descarte_fuera_de_turno", false, [&] {
            return juego->intentarDescarteFueraDeTurno(static_cast<size_t>(idJugador), static_cast<size_t>(posMano));
        });
    }

    FFI_EXPORT void forzar_jugador_actual(Juego* juego, int idx) {
        if (!juego || neg(idx)) return;
        seguroVoid("forzar_jugador_actual", [&] { juego->forzarJugadorActual(static_cast<size_t>(idx)); });
    }

    FFI_EXPORT void ejecutar_sorteo_orden_inicial(Juego* juego) {
        if (!juego) return;
        seguroVoid("ejecutar_sorteo_orden_inicial", [&] { juego->ejecutarSorteoOrdenInicial(); });
    }

    FFI_EXPORT int obtener_valor_sorteo_orden_inicial(Juego* juego, int idxJugador) {
        if (!juego || neg(idxJugador)) return -99;
        return seguro<int>("obtener_valor_sorteo_orden_inicial", -99, [&] {
            return juego->valorSorteoOrdenInicial(static_cast<size_t>(idxJugador));
        });
    }

    // ---------------- EFECTOS ----------------

    FFI_EXPORT bool usar_efecto_carta_robada(Juego* juego) {
        if (!juego) return false;
        return seguro<bool>("usar_efecto_carta_robada", false, [&] { return juego->usarEfectoCartaRobada(); });
    }

    FFI_EXPORT int efecto_mirar_carta(Juego* juego, int posMano) {
        if (!juego || neg(posMano)) return -1;
        return seguro<int>("efecto_mirar_carta", -1, [&] {
            auto opt = juego->efectoMirarCarta(static_cast<size_t>(posMano));
            return opt ? static_cast<int>(*opt) : -1;
        });
    }

    FFI_EXPORT bool efecto_intercambiar_ciegas(Juego* juego, int posPropia, int idxRival, int posRival) {
        if (!juego || neg(posPropia) || neg(idxRival) || neg(posRival)) return false;
        return seguro<bool>("efecto_intercambiar_ciegas", false, [&] {
            return juego->efectoIntercambiarCiegas(static_cast<size_t>(posPropia),
                                                   static_cast<size_t>(idxRival),
                                                   static_cast<size_t>(posRival));
        });
    }

    FFI_EXPORT int efecto_intercambiar_mirando(Juego* juego, int posPropia, int idxRival, int posRival) {
        if (!juego || neg(posPropia) || neg(idxRival) || neg(posRival)) return -1;
        return seguro<int>("efecto_intercambiar_mirando", -1, [&] {
            auto opt = juego->efectoIntercambiarMirando(static_cast<size_t>(posPropia),
                                                        static_cast<size_t>(idxRival),
                                                        static_cast<size_t>(posRival));
            return opt ? static_cast<int>(*opt) : -1;
        });
    }

    FFI_EXPORT bool efecto_intercambiar_manos(Juego* juego, int idxJugador1, int idxJugador2) {
        if (!juego || neg(idxJugador1) || neg(idxJugador2)) return false;
        return seguro<bool>("efecto_intercambiar_manos", false, [&] {
            return juego->efectoIntercambiarManos(static_cast<size_t>(idxJugador1), static_cast<size_t>(idxJugador2));
        });
    }

    FFI_EXPORT bool efecto_descartar_propia(Juego* juego, int posMano) {
        if (!juego || neg(posMano)) return false;
        return seguro<bool>("efecto_descartar_propia", false, [&] {
            return juego->efectoDescartarPropia(static_cast<size_t>(posMano));
        });
    }

    FFI_EXPORT bool penalizar_jugador(Juego* juego, int idJugador) {
        if (!juego || neg(idJugador)) return false;
        return seguro<bool>("penalizar_jugador", false, [&] { return juego->penalizarJugador(static_cast<size_t>(idJugador)); });
    }

    // ---------------- SERIALIZACIÓN ----------------

    // El llamador DEBE liberar con liberar_string_serializado().
    FFI_EXPORT const char* serializar_estado(Juego* juego) {
        if (!juego) return nullptr;
        return seguro<const char*>("serializar_estado", nullptr, [&]() -> const char* {
            std::string estado = juego->serializarEstado();
            char* buffer = new (std::nothrow) char[estado.size() + 1];
            if (!buffer) return nullptr;
            estado.copy(buffer, estado.size());
            buffer[estado.size()] = '\0';
            return buffer;
        });
    }

    FFI_EXPORT void liberar_string_serializado(const char* ptr) {
        delete[] ptr;
    }

    // Los datos vienen de almacenamiento local: pueden estar corruptos.
    FFI_EXPORT bool restaurar_estado(Juego* juego, const char* datos) {
        if (!juego || !datos) return false;
        return seguro<bool>("restaurar_estado", false, [&] { return juego->restaurarEstado(std::string(datos)); });
    }

    // ---------------- IA ----------------

    FFI_EXPORT IA* crear_ia() {
        return seguro<IA*>("crear_ia", nullptr, [] { return new IA(); });
    }

    FFI_EXPORT void destruir_ia(IA* ia) {
        seguroVoid("destruir_ia", [&] { delete ia; });
    }

    FFI_EXPORT void ia_reiniciar_memoria(IA* ia, int numCartas) {
        if (!ia || neg(numCartas)) return;
        seguroVoid("ia_reiniciar_memoria", [&] { ia->reiniciarMemoria(static_cast<size_t>(numCartas)); });
    }

    FFI_EXPORT void ia_recordar_carta_propia(IA* ia, int pos, int valorCarta) {
        if (!ia || neg(pos)) return;
        seguroVoid("ia_recordar_carta_propia", [&] {
            ia->recordarCartaPropia(static_cast<size_t>(pos), static_cast<Carta>(valorCarta));
        });
    }

    FFI_EXPORT void ia_olvidar_carta_propia(IA* ia, int pos) {
        if (!ia || neg(pos)) return;
        seguroVoid("ia_olvidar_carta_propia", [&] { ia->olvidarCartaPropia(static_cast<size_t>(pos)); });
    }

    FFI_EXPORT void ia_actualizar_memoria_tras_intercambio(IA* ia, Juego* juego, int idxPropio) {
        if (!ia || !juego || neg(idxPropio)) return;
        seguroVoid("ia_actualizar_memoria_tras_intercambio", [&] {
            ia->actualizarMemoriaTrasIntercambio(*juego, static_cast<size_t>(idxPropio));
        });
    }

    FFI_EXPORT FfiDecisionAccion ia_decidir_accion(IA* ia, Juego* juego, int idxPropio) {
        FfiDecisionAccion porDefecto{0, 0};
        if (!ia || !juego || neg(idxPropio)) return porDefecto;
        return seguro<FfiDecisionAccion>("ia_decidir_accion", porDefecto, [&] {
            auto d = ia->decidirAccion(*juego, static_cast<size_t>(idxPropio));
            FfiDecisionAccion r{};
            r.accion = static_cast<int>(d.accion);
            r.posManoIntercambio = d.posManoIntercambio;
            return r;
        });
    }

    FFI_EXPORT int ia_decidir_posicion_mirar_propia(IA* ia, Juego* juego, int idxPropio) {
        if (!ia || !juego || neg(idxPropio)) return 0;
        return seguro<int>("ia_decidir_posicion_mirar_propia", 0, [&] {
            return static_cast<int>(ia->decidirPosicionMirarPropia(*juego, static_cast<size_t>(idxPropio)));
        });
    }

    FFI_EXPORT int ia_decidir_posicion_descartar_propia(IA* ia, Juego* juego, int idxPropio) {
        if (!ia || !juego || neg(idxPropio)) return 0;
        return seguro<int>("ia_decidir_posicion_descartar_propia", 0, [&] {
            return static_cast<int>(ia->decidirPosicionDescartarPropia(*juego, static_cast<size_t>(idxPropio)));
        });
    }

    FFI_EXPORT bool ia_decidir_cierre(IA* ia, Juego* juego, int idxPropio) {
        if (!ia || !juego || neg(idxPropio)) return false;
        return seguro<bool>("ia_decidir_cierre", false, [&] {
            return ia->decidirCierre(*juego, static_cast<size_t>(idxPropio));
        });
    }

    FFI_EXPORT FfiDecisionIntercambioRival ia_decidir_intercambio_rival(IA* ia, Juego* juego, int idxPropio) {
        FfiDecisionIntercambioRival porDefecto{0, 0, 0};
        if (!ia || !juego || neg(idxPropio)) return porDefecto;
        return seguro<FfiDecisionIntercambioRival>("ia_decidir_intercambio_rival", porDefecto, [&] {
            auto d = ia->decidirIntercambioRival(*juego, static_cast<size_t>(idxPropio));
            FfiDecisionIntercambioRival r{};
            r.posPropia = d.posPropia;
            r.idxRival = d.idxRival;
            r.posRival = d.posRival;
            return r;
        });
    }

    FFI_EXPORT int ia_decidir_descarte_rapido(IA* ia, int valorCartaDescarte) {
        if (!ia || valorCartaDescarte == -1) return -1;
        return seguro<int>("ia_decidir_descarte_rapido", -1, [&] {
            auto opt = ia->decidirDescarteRapido(static_cast<Carta>(valorCartaDescarte));
            return opt ? static_cast<int>(*opt) : -1;
        });
    }

    FFI_EXPORT void ia_eliminar_carta_propia(IA* ia, int pos) {
        if (!ia || neg(pos)) return;
        seguroVoid("ia_eliminar_carta_propia", [&] { ia->eliminarCartaPropia(static_cast<size_t>(pos)); });
    }

    FFI_EXPORT int ia_decidir_rival_para_intercambio(IA* ia, Juego* juego, int idxPropio) {
        if (!ia || !juego || neg(idxPropio)) return idxPropio;
        return seguro<int>("ia_decidir_rival_para_intercambio", idxPropio, [&] {
            return static_cast<int>(ia->decidirRivalParaIntercambio(*juego, static_cast<size_t>(idxPropio)));
        });
    }

    FFI_EXPORT void ia_establecer_modo_determinista(IA* ia, bool activo) {
        if (!ia) return;
        seguroVoid("ia_establecer_modo_determinista", [&] { ia->establecerModoDeterminista(activo); });
    }
}