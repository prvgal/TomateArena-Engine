/*
 * Juego.hpp
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

#ifndef JUEGO_HPP_
#define JUEGO_HPP_

#include "../include/tipos.hpp"
#include "../include/Jugador.hpp"

#include <vector>
#include <stack>
#include <optional>
#include <string>

constexpr size_t NUM_PALOS = 4;
constexpr size_t NUM_VALORES = 12;
constexpr size_t CARTAS_MANO = 4;
constexpr size_t NUM_JOKERS = 2;
constexpr size_t NUM_DESCARTES = 2;
constexpr size_t UMBRAL_CARTAS_SIN_PENALIZACION = 7;

class Juego {
public:
    Juego();
    void iniciar(size_t numJugadores = 4);

    std::optional<CartaRobada> cartaDescarte() const 
    { return (descarte_.empty()) ? std::nullopt : std::optional<CartaRobada>{descarte_.top()}; }

    const Jugador& jugador(size_t idx) const { return jugadores_[idx]; }

    // Permite consultar una carta puntual en el revelado inicial o por efectos
    std::optional<Carta> consultarCartaMano(size_t idxJugador, size_t posMano) const;
    void terminarFaseRevelado();

    // estados del juego
    bool esFin() const { return estado_ == EstadoJuego::FIN_PARTIDA; }
    EstadoJuego estado() const { return estado_; }
    size_t jugadorActual() const { return jugadorActual_; }
    std::optional<CartaRobada> cartaRobada() const { return cartaRobada_; }
    bool cierre() const { return cierre_; }
    bool cartaRobadaEsJoker() const { return cartaRobada_ && cartaRobada_->tipo == TipoCartaRobada::JOKER; }
    bool cartaRobadaEsDescarte() const { return cartaRobada_ && cartaRobada_->tipo == TipoCartaRobada::DESCARTE; } 
    std::optional<Carta> valorCartaRobada() const {
        if (!cartaRobada_ || cartaRobada_->tipo != TipoCartaRobada::CARTA)
            return std::nullopt;
        return cartaRobada_->carta;
    }
    bool descarteEsReaccionable() const 
    { return !descarte_.empty() && descarte_.top().tipo == TipoCartaRobada::CARTA; }

    bool huboPenalizacionUltimoIntento() const { return huboPenalizacionUltimoIntento_; }

    // consultas del juego
    std::vector<int> obtenerPuntuaciones() const;
    std::optional<size_t> obtenerGanador(bool soloHumanoDesempata = false) const;
    int ultimoCriterioDesempate() const;
    size_t numeroJugadores() const { return jugadores_.size(); }
    bool huboRebarajeUltimoRobo() const;

    // acciones
    bool robarDelMonton();
    bool descartarCartaRobada();
    bool intercambiarCartaRobada(size_t posMano);
    bool cerrarPartida(size_t idxJugador);

    bool penalizarJugador(size_t idxJugador);
    bool intentarDescarteFueraDeTurno(size_t idxJugador, size_t posMano);

    void forzarJugadorActual(size_t idx);

    void ejecutarSorteoOrdenInicial();
    int valorSorteoOrdenInicial(size_t idxJugador) const;

    // acciones especiales (llamadas efecto)
    bool usarEfectoCartaRobada();
    std::optional<Carta> efectoMirarCarta(size_t posMano);  // habilidad 10
    bool efectoIntercambiarCiegas(size_t posMano, size_t idxRival, size_t posRival);  // habilidad 11
    std::optional<Carta> efectoIntercambiarMirando(size_t posMano, size_t idxRival, size_t posRival);   // habilidad 12
    bool efectoIntercambiarManos(size_t idx1, size_t idx2);  // habilidad joker
    bool efectoDescartarPropia(size_t posMano);

    // Vuelca TODO el estado interno necesario para reanudar la
    // partida exactamente donde se dejó: montón, descarte, mano de
    // cada jugador, turno actual, fase, cierre, carta robada
    // pendiente, resultado del sorteo inicial, etc.
    std::string serializarEstado() const;

    // Reconstruye el estado a partir de serializarEstado()
    bool restaurarEstado(const std::string& datos);

    virtual ~Juego() = default;
protected:
    // Construye las NUM_PALOS * NUM_VALORES cartas numéricas SIN
    // barajar (mismo bucle palo/valor que había inline en iniciar()).
    std::vector<CartaRobada> construirCartasNormales() const;

    // decide el orden final de las cartas
    // numéricas antes de repartir manos. Por defecto baraja
    // aleatoriamente
    virtual std::vector<CartaRobada> generarCartasNormales() const;

    // decide el orden final del montón (numéricas
    // sobrantes + jokers + cartas de Descarte) antes de empezar a
    // jugar. Por defecto baraja aleatoriamente
    virtual std::vector<CartaRobada> mezclarMazoFinal(std::vector<CartaRobada> mazo) const;    
private:
    void avanzarTurno();
    void reponerMontonDesdeDescarte();

    // --- Helpers de serialización (ver serializarEstado/restaurarEstado) ---
    std::vector<CartaRobada> obtenerPilaComoVector(const std::stack<CartaRobada>& pila) const;
    static std::stack<CartaRobada> construirPilaDesdeVector(const std::vector<CartaRobada>& vec);
    static std::string codificarCartaRobada(const CartaRobada& c);
    static bool decodificarCartaRobada(const std::string& token, CartaRobada& out);

    std::stack<CartaRobada> monton_;
    std::stack<CartaRobada> descarte_;
    std::vector<Jugador> jugadores_;

    size_t jugadorActual_;
    EstadoJuego estado_;
    std::optional<CartaRobada> cartaRobada_;
    size_t turnosRestantes_;
    bool cierre_;
    std::optional<size_t> ganadorPorCartas_;
    size_t ultimoJugadorQueActuo_ = static_cast<size_t>(-1);
    bool huboRebarajeUltimoRobo_ = false;
    bool huboPenalizacionUltimoIntento_ = false;
    std::vector<int> ultimoSorteoOrdenInicial_;

    std::optional<size_t> jugadorQueCerro_;
    std::vector<int> puntuacionesIniciales_;
    std::vector<size_t> cartasEspecialesUsadas_;
    mutable int ultimoCriterioDesempate_ = 0;
};

#endif // JUEGO_HPP_