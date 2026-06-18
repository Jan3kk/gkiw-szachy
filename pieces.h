#ifndef PIECES_H
#define PIECES_H

// Proceduralne generowanie geometrii bierek szachowych.
// Figury obrotowo-symetryczne (pionek, goniec, wieza, hetman, krol) tworzone
// jako BRYLY OBROTOWE (lathe): profil (promien, wysokosc) obracany wokol osi Y.
// Skoczek skladany jest w main z prostych bryl (szescianow) + podstawy.
// Kazdy wierzcholek ma pozycje i wektor normalny (do oswietlenia).

#include <vector>
#include <cmath>
#include <glm/glm.hpp>
#include "constants.h"

struct Mesh {
    std::vector<float> verts;   // x,y,z na wierzcholek
    std::vector<float> norms;   // nx,ny,nz na wierzcholek
    std::vector<float> tex;     // u,v (opcjonalne - tekstury)
    std::vector<float> tans;    // tx,ty,tz (opcjonalne - wektor styczny do normal mappingu)
    int count = 0;              // liczba wierzcholkow
};

inline void pushVN(Mesh& m, const glm::vec3& p, const glm::vec3& n) {
    m.verts.push_back(p.x); m.verts.push_back(p.y); m.verts.push_back(p.z);
    m.norms.push_back(n.x); m.norms.push_back(n.y); m.norms.push_back(n.z);
}

// Jak pushVN, ale dodatkowo zapisuje wspolrzedne tekstury i wektor styczny
inline void pushVNTT(Mesh& m, const glm::vec3& p, const glm::vec3& n, const glm::vec2& uv, const glm::vec3& t) {
    m.verts.push_back(p.x); m.verts.push_back(p.y); m.verts.push_back(p.z);
    m.norms.push_back(n.x); m.norms.push_back(n.y); m.norms.push_back(n.z);
    m.tex.push_back(uv.x); m.tex.push_back(uv.y);
    m.tans.push_back(t.x); m.tans.push_back(t.y); m.tans.push_back(t.z);
}

// Bryla obrotowa z profilu prof[i] = (promien, wysokosc), obracanego w 'seg' krokach.
// Normalne liczone analitycznie z nachylenia profilu (gladkie cieniowanie).
inline Mesh makeLathe(const std::vector<glm::vec2>& prof, int seg) {
    Mesh m;
    int N = (int)prof.size();
    if (N < 2) return m;

    // Dwuwymiarowa normalna (w plaszczyznie promien-wysokosc) w kazdym punkcie profilu
    std::vector<glm::vec2> n2(N);
    for (int i = 0; i < N; i++) {
        glm::vec2 t;
        if (i == 0)            t = prof[1] - prof[0];
        else if (i == N - 1)   t = prof[N - 1] - prof[N - 2];
        else                   t = prof[i + 1] - prof[i - 1];
        glm::vec2 nn(t.y, -t.x); // prostopadla do stycznej, skierowana na zewnatrz
        float L = std::sqrt(nn.x * nn.x + nn.y * nn.y);
        if (L > 1e-6f) nn /= L; else nn = glm::vec2(1.0f, 0.0f);
        n2[i] = nn;
    }

    for (int i = 0; i < N - 1; i++) {
        for (int j = 0; j < seg; j++) {
            float a0 = 2.0f * PI * j / seg;
            float a1 = 2.0f * PI * (j + 1) / seg;
            float c0 = cosf(a0), s0 = sinf(a0), c1 = cosf(a1), s1 = sinf(a1);

            glm::vec3 p00(prof[i].x * c0, prof[i].y, prof[i].x * s0);
            glm::vec3 p10(prof[i + 1].x * c0, prof[i + 1].y, prof[i + 1].x * s0);
            glm::vec3 p11(prof[i + 1].x * c1, prof[i + 1].y, prof[i + 1].x * s1);
            glm::vec3 p01(prof[i].x * c1, prof[i].y, prof[i].x * s1);

            glm::vec3 N00(n2[i].x * c0, n2[i].y, n2[i].x * s0);
            glm::vec3 N10(n2[i + 1].x * c0, n2[i + 1].y, n2[i + 1].x * s0);
            glm::vec3 N11(n2[i + 1].x * c1, n2[i + 1].y, n2[i + 1].x * s1);
            glm::vec3 N01(n2[i].x * c1, n2[i].y, n2[i].x * s1);

            // UV: u wokol osi, v wzdluz profilu; tangent = kierunek "wokol osi"
            float u0 = (float)j / seg, u1 = (float)(j + 1) / seg;
            float v0 = (float)i / (N - 1), v1 = (float)(i + 1) / (N - 1);
            glm::vec3 t0(-s0, 0.0f, c0), t1(-s1, 0.0f, c1);

            pushVNTT(m, p00, N00, glm::vec2(u0, v0), t0);
            pushVNTT(m, p10, N10, glm::vec2(u0, v1), t0);
            pushVNTT(m, p11, N11, glm::vec2(u1, v1), t1);
            pushVNTT(m, p00, N00, glm::vec2(u0, v0), t0);
            pushVNTT(m, p11, N11, glm::vec2(u1, v1), t1);
            pushVNTT(m, p01, N01, glm::vec2(u1, v0), t1);
        }
    }
    m.count = (int)m.verts.size() / 3;
    return m;
}

// Kula o promieniu r - jako bryla obrotowa z polokregu
inline Mesh makeSphere(float r, int stacks, int sectors) {
    std::vector<glm::vec2> prof;
    for (int i = 0; i <= stacks; i++) {
        float a = -PI / 2.0f + PI * (float)i / stacks;
        prof.push_back(glm::vec2(r * cosf(a), r * sinf(a)));
    }
    return makeLathe(prof, sectors);
}

// Prostopadloscian sx,sy,sz wysrodkowany w (0,0,0): pozycje, normalne, UV i wektory styczne.
// Tangent na kazdej scianie wskazuje kierunek rosnacego U (potrzebny do normal mappingu).
inline Mesh makeBox(float sx, float sy, float sz) {
    Mesh m;
    float x = sx * 0.5f, y = sy * 0.5f, z = sz * 0.5f;
    glm::vec3 nPX(1, 0, 0), nNX(-1, 0, 0), nPY(0, 1, 0), nNY(0, -1, 0), nPZ(0, 0, 1), nNZ(0, 0, -1);
    glm::vec3 tPX(0, 0, 1), tNX(0, 0, -1), tPY(1, 0, 0), tNY(1, 0, 0), tPZ(1, 0, 0), tNZ(-1, 0, 0);
    // +X
    pushVNTT(m, glm::vec3(x, -y, -z), nPX, glm::vec2(0, 0), tPX);
    pushVNTT(m, glm::vec3(x, -y, z), nPX, glm::vec2(1, 0), tPX);
    pushVNTT(m, glm::vec3(x, y, z), nPX, glm::vec2(1, 1), tPX);
    pushVNTT(m, glm::vec3(x, -y, -z), nPX, glm::vec2(0, 0), tPX);
    pushVNTT(m, glm::vec3(x, y, z), nPX, glm::vec2(1, 1), tPX);
    pushVNTT(m, glm::vec3(x, y, -z), nPX, glm::vec2(0, 1), tPX);
    // -X
    pushVNTT(m, glm::vec3(-x, -y, z), nNX, glm::vec2(0, 0), tNX);
    pushVNTT(m, glm::vec3(-x, -y, -z), nNX, glm::vec2(1, 0), tNX);
    pushVNTT(m, glm::vec3(-x, y, -z), nNX, glm::vec2(1, 1), tNX);
    pushVNTT(m, glm::vec3(-x, -y, z), nNX, glm::vec2(0, 0), tNX);
    pushVNTT(m, glm::vec3(-x, y, -z), nNX, glm::vec2(1, 1), tNX);
    pushVNTT(m, glm::vec3(-x, y, z), nNX, glm::vec2(0, 1), tNX);
    // +Y
    pushVNTT(m, glm::vec3(-x, y, z), nPY, glm::vec2(0, 1), tPY);
    pushVNTT(m, glm::vec3(x, y, z), nPY, glm::vec2(1, 1), tPY);
    pushVNTT(m, glm::vec3(x, y, -z), nPY, glm::vec2(1, 0), tPY);
    pushVNTT(m, glm::vec3(-x, y, z), nPY, glm::vec2(0, 1), tPY);
    pushVNTT(m, glm::vec3(x, y, -z), nPY, glm::vec2(1, 0), tPY);
    pushVNTT(m, glm::vec3(-x, y, -z), nPY, glm::vec2(0, 0), tPY);
    // -Y
    pushVNTT(m, glm::vec3(-x, -y, -z), nNY, glm::vec2(0, 1), tNY);
    pushVNTT(m, glm::vec3(x, -y, -z), nNY, glm::vec2(1, 1), tNY);
    pushVNTT(m, glm::vec3(x, -y, z), nNY, glm::vec2(1, 0), tNY);
    pushVNTT(m, glm::vec3(-x, -y, -z), nNY, glm::vec2(0, 1), tNY);
    pushVNTT(m, glm::vec3(x, -y, z), nNY, glm::vec2(1, 0), tNY);
    pushVNTT(m, glm::vec3(-x, -y, z), nNY, glm::vec2(0, 0), tNY);
    // +Z
    pushVNTT(m, glm::vec3(-x, -y, z), nPZ, glm::vec2(0, 0), tPZ);
    pushVNTT(m, glm::vec3(x, -y, z), nPZ, glm::vec2(1, 0), tPZ);
    pushVNTT(m, glm::vec3(x, y, z), nPZ, glm::vec2(1, 1), tPZ);
    pushVNTT(m, glm::vec3(-x, -y, z), nPZ, glm::vec2(0, 0), tPZ);
    pushVNTT(m, glm::vec3(x, y, z), nPZ, glm::vec2(1, 1), tPZ);
    pushVNTT(m, glm::vec3(-x, y, z), nPZ, glm::vec2(0, 1), tPZ);
    // -Z
    pushVNTT(m, glm::vec3(x, -y, -z), nNZ, glm::vec2(0, 0), tNZ);
    pushVNTT(m, glm::vec3(-x, -y, -z), nNZ, glm::vec2(1, 0), tNZ);
    pushVNTT(m, glm::vec3(-x, y, -z), nNZ, glm::vec2(1, 1), tNZ);
    pushVNTT(m, glm::vec3(x, -y, -z), nNZ, glm::vec2(0, 0), tNZ);
    pushVNTT(m, glm::vec3(-x, y, -z), nNZ, glm::vec2(1, 1), tNZ);
    pushVNTT(m, glm::vec3(x, y, -z), nNZ, glm::vec2(0, 1), tNZ);
    m.count = (int)m.verts.size() / 3;
    return m;
}

// ----- profile poszczegolnych bierek (promien, wysokosc) od dolu do gory -----
#define SEG 28

inline Mesh makePawn() {
    std::vector<glm::vec2> p = {
        {0.00f,0.00f},{0.33f,0.00f},{0.33f,0.05f},{0.29f,0.09f},{0.20f,0.13f},
        {0.13f,0.20f},{0.12f,0.34f},{0.17f,0.40f},{0.10f,0.46f},
        {0.19f,0.55f},{0.21f,0.66f},{0.16f,0.77f},{0.00f,0.88f}
    };
    return makeLathe(p, SEG);
}

inline Mesh makeBishop() {
    std::vector<glm::vec2> p = {
        {0.00f,0.00f},{0.34f,0.00f},{0.34f,0.05f},{0.30f,0.09f},{0.21f,0.14f},
        {0.14f,0.22f},{0.12f,0.45f},{0.18f,0.52f},{0.10f,0.58f},
        {0.17f,0.66f},{0.20f,0.80f},{0.18f,0.95f},{0.12f,1.05f},
        {0.13f,1.10f},{0.07f,1.16f},{0.00f,1.20f}
    };
    return makeLathe(p, SEG);
}

inline Mesh makeRook() {
    std::vector<glm::vec2> p = {
        {0.00f,0.00f},{0.36f,0.00f},{0.36f,0.06f},{0.31f,0.10f},{0.24f,0.16f},
        {0.23f,0.60f},{0.30f,0.66f},{0.30f,0.74f},{0.00f,0.74f}
    };
    return makeLathe(p, SEG);
}

inline Mesh makeQueen() {
    std::vector<glm::vec2> p = {
        {0.00f,0.00f},{0.37f,0.00f},{0.37f,0.06f},{0.32f,0.10f},{0.22f,0.16f},
        {0.15f,0.26f},{0.13f,0.55f},{0.19f,0.62f},{0.11f,0.68f},
        {0.18f,0.80f},{0.23f,1.00f},{0.25f,1.15f},{0.22f,1.25f},
        {0.27f,1.32f},{0.30f,1.40f},{0.26f,1.45f},{0.00f,1.46f}
    };
    return makeLathe(p, SEG);
}

inline Mesh makeKing() {
    std::vector<glm::vec2> p = {
        {0.00f,0.00f},{0.38f,0.00f},{0.38f,0.06f},{0.33f,0.10f},{0.23f,0.16f},
        {0.16f,0.26f},{0.14f,0.60f},{0.20f,0.67f},{0.12f,0.73f},
        {0.19f,0.86f},{0.24f,1.06f},{0.26f,1.22f},{0.22f,1.33f},
        {0.27f,1.40f},{0.30f,1.48f},{0.24f,1.53f},{0.20f,1.57f},{0.00f,1.58f}
    };
    return makeLathe(p, SEG);
}

// Podstawa skoczka (reszte skladamy z bryl w main)
inline Mesh makeKnightBase() {
    std::vector<glm::vec2> p = {
        {0.00f,0.00f},{0.34f,0.00f},{0.34f,0.05f},{0.30f,0.09f},
        {0.22f,0.14f},{0.17f,0.20f},{0.16f,0.30f}
    };
    return makeLathe(p, SEG);
}

#endif
