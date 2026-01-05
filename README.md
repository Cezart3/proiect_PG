# 3D Space Drone Simulation - Project Documentation

## 1. Descrierea Proiectului

Acest proiect implementează o simulare fotorealistă a unei drone care explorează un mediu spațial extraterestru. Scena este construită utilizând OpenGL 4.1 și biblioteci asociate (GLFW, GLM, GLEW), demonstrând tehnici avansate de randare și simulare fizică.

## 2. Funcționalități Implementate

### Vizualizare și Control (2p)

- **Cameră Third-Person**: Camera urmărește drona fluid (Lerp smoothing) din spate.
- **Cinematic Mode (Tasta I)**: O animație automată care plimbă camera prin puncte cheie ale scenei (orbită, asteroizi, sol) folosind interpolare.
- **Control Dronă**:
  - `W/S`: Accelerare înainte/înapoi.
  - `A/D`: Înclinare stânga/dreapta (Banking).
  - `SPACE/SHIFT`: Urcare/Coborâre (VTOL).
  - `Săgeți`: Rotire și Pitch manual.
- **Control Lumină**: `J/L` rotește soarele în jurul scenei.

### Iluminare și Materiale (2p)

- **Modele de Iluminare**: Blinn-Phong.
- **Surse de Lumină**:
  1.  **Lumină Direcțională (Soare)**: Generează umbre dinamice.
  2.  **Spot Light (Lanterna Dronei)**: Proiectează lumină în fața dronei (Toggle `F`).
  3.  **Point Lights**: Cristale luminiscente (Verzi) în cratere.
- **Materiale**: Suport complet pentru ambient (`Ka`), difuz (`Kd`), specular (`Ks`) și texturi multiple.

### Moduri de Vizualizare (0.5p)

Tastele `1-4` schimbă modul de randare în timp real:

- `1`: **Wireframe** (Doar muchii).
- `2`: **Solid Smooth** (Randare normală).
- `3`: **Solid Polygonal** (Aspect "Low Poly", fără texturi, normale fațetate).
- `4`: **Points** (Nori de puncte).

### Umbre și Efecte Avansate (4p+)

- **Shadow Mapping**: Umbre dinamice cu rezoluție 4096p.
- **Ceață Atmosferică**: Ceață volumetrică exponențială (`exp(-dist^2)`), controlabilă (`C`).
- **Sistem de Particule**: Simulare ploaie spațială/praf (`P`) folosind `GL_LINES`.
- **Skybox High-Res**: Cubemap 4K cu nebuloase.

### Fizică și Interacțiune

- **Coliziuni Complexe**:
  - Sferă-Sferă pentru asteroizi și pietre.
  - Cilindru-Sferă pentru turnuri (Spires).
- **Mediu Dinamic**: Asteroizi care orbitează și se rotesc independent.
- **Crash Mechanics**: Dacă drona lovește un obstacol, motorul cedează, iar drona se prăbușește rotațional (tumble) până la sol.

## 3. Detalii Tehnice

### Structura Proiectului

- `main.cpp`: Bucla principală, input, randare scenă.
- `Drone.cpp/hpp`: Logica fizică a dronei, stări (zbor/prăbușire).
- `World.cpp/hpp`: Gestionarea mediului (generare procedurală asteroizi, coliziuni).
- `Camera.cpp/hpp`: Sistemul de cameră (Follow & Presentation).
- `ParticleSystem.cpp/hpp`: Sistem eficient de particule (reutilizare buffer).

### Algoritmi Cheie

1.  **Calculul Umbrelor**: Randare în două treceri (Depth & Lighting Pass). Matricea de lumină este centrată pe jucător pentru a maximiza rezoluția umbrelor în apropiere.
2.  **Flat Shading**: Pentru modul `3`, normalele sunt recalculate în fragment shader folosind `cross(dFdx(pos), dFdy(pos))` pentru a obține aspectul fațetat.
3.  **Coliziune**: Verificare în doi pași (Activă - predicție poziție viitoare, Pasivă - respingere dacă obiectul intră în dronă).

## 4. Controale

| Tastă         | Acțiune                                |
| ------------- | -------------------------------------- |
| `W/A/S/D`     | Mișcare Dronă                          |
| `SPACE/SHIFT` | Sus/Jos                                |
| `I`           | Start/Stop Mod Prezentare (Cinematic)  |
| `P`           | Start/Stop Ploaie                      |
| `C`           | Start/Stop Ceață                       |
| `F`           | Lanterna Dronă                         |
| `1/2/3/4`     | Moduri Randare (Wire/Solid/Poly/Point) |
| `J/L`         | Rotire Soare                           |

## 5. Bibliografie

- Cursuri PG (Universitatea Transilvania)
- LearnOpenGL.com
- Modele 3D: Kenney Space Kit
