// Astronautilius retro space shooter game inspired from original atari game "Asteroids" (https://www.youtube.com/watch?v=WYSupJ5r2zo)
/*
Formal Credits:
- rotation vectors in 2D: https://stackoverflow.com/questions/17410809/how-to-calculate-rotation-in-2d-in-javascript
- random float generation: https://www.sololearn.com/Discuss/280755/random-floats-in-c
- point inside polygon: https://stackoverflow.com/questions/11716268/point-in-polygon-algorithm
*/

#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<conio.h>
#include <windows.h>
#include <mmsystem.h>
#include<time.h>
#include "lib/CC212VSGL.h"

#pragma comment(lib, "winmm.lib")

// Core elements
#define WIDTH 800
#define HEIGHT 600
#define PI (atan(1) * 4)

// Game limits
// Ship
#define SHIP_MAX_VELOCITY 15
#define SHIP_ROT_SPEED 3

// Asteroid
#define ASTEROID_DEFAULT_RATE 5
#define ASTEROID_DEFAULT_SCALE 5.0
#define ASTEROID_DEFAULT_HEALTH 1
#define ASTEROID_ROT_SPEED 0.1
#define ASTEROID_MAX_ROT_SPEED 1.0
#define ASTEROID_MAX_VELOCITY 0.5

// Explosion
#define EXPLOSION_DEFAULT_SIZE 10
#define EXPLOSION_DEFAULT_AGE 500

typedef struct {
    float x;
    float y;
} Vector2;

typedef struct {
    Vector2 pos;
    Vector2 velocity;
    float velocityTotal;
} Entity;

typedef struct {
    Vector2 model[5] = {
        {0, -20}, // Cockpit
        {10, 10}, // Right wing
        {0, 0}, // Center
        {-10, 10}, // Left wing
        {0, -20}, // Cockpit
    };
    Vector2 edges[5];
    Entity entity;
    float angle = 90;
    float thrust = 0;
    bool airbrake = false;
} Ship;

typedef struct {
    bool intro = true;
    bool alive = false;
    bool exploding = false;
    bool gameOver = false;
    int level = 1;
    int score = 0;
    int previousHighScore = 0;
    int shots = 0;
    int gameOverTime = 0;
} Stats;

typedef struct {
    Entity entity;
    Vector2 model[8];
    Vector2 edges[8];
    float angle = 0;
    float vangle = 0; 
    float health = 0;
    int size = 0;
} Asteroid;

typedef struct {
    Entity entity;
    float dx = 0.0;
    float dy = 0.0;
    int birth;
    int lifespan;
} Shot;

typedef struct {
    Entity entity;
    int size = 5;
    int age = 2;
} Explosion;

Stats stats;
Explosion explosions[100];
Shot shots[100];
int num_of_explosions = 0, shotsCounter = 0;
float consoleFPS = 0;

float random_float(float min, float max) {
    return ((float)rand() / RAND_MAX) * (max - min) + min;
}

int getCurrentTimeInMS() {
    SYSTEMTIME time;
    GetSystemTime(&time);
    return time.wMilliseconds;
}

void ReadKeyboard(bool* keys) {
    for (int x = 0; x < 256; x++) {
        keys[x] = GetKeyState(x) & 0x800;
    }
}

Vector2 rotateVector(float x, float y, float angle) {
    float radians = (PI / 180) * angle;
    float s = sin(radians);
    float c = cos(radians);
    Vector2 point = { (c * x) - (s * y), (s * x) + (c * y) };
    return point;
}


Shot createShot(float x, float y, float vx, float vy, int lifespan = 500) {
    SYSTEMTIME time;
    GetSystemTime(&time);
    Shot newShot;
    newShot.entity.pos = { x, y };
    newShot.entity.velocity = { vx, vy };
    newShot.dx = fabs(vx) > fabs(vy) ? 1.0 : vx / vy;
    newShot.dy = fabs(vy) > fabs(vx) ? 1.0 : vy / vx;
    newShot.birth = time.wMilliseconds;
    newShot.lifespan = lifespan;
    return newShot;
}

Asteroid createAsteroid(float marginY, float marginX, int size, float x = floor(random_float(0, WIDTH / 4 + random_float(0, 1) * WIDTH / 2)), float y = floor(random_float(0, HEIGHT / 4 + random_float(0, 1) * HEIGHT / 2)), float vx = ASTEROID_MAX_VELOCITY - random_float(0, 0.4), float vy = ASTEROID_MAX_VELOCITY - random_float(0, 0.4), float vangle = random_float(ASTEROID_ROT_SPEED, ASTEROID_MAX_ROT_SPEED)) {
    Asteroid newAster;
    int angles, i;
    newAster.size = size;
    newAster.vangle = vangle;
    newAster.entity.pos.x = x + marginX;
    newAster.entity.pos.y = y + marginY;
    newAster.entity.velocity.x = vx;
    newAster.entity.velocity.y = vy;
    newAster.health = ASTEROID_DEFAULT_HEALTH + floor(size / 2);
    for (angles = 0, i = 0; angles < 360; angles += 45, i++) {
        Vector2 point = rotateVector(0.0, ASTEROID_DEFAULT_SCALE + (random_float(0, 1) * (size < 50 ? size : size / 2)), angles);
        newAster.model[i] = point;
    }
    return newAster;
}

Explosion createExplosion(float x, float y, int size) {
    Explosion explosion;
    explosion.entity.pos = { x, y };
    explosion.size = size;
    return explosion;
}

void destroyAsteroid(Asteroid asteriods[], int& num_of_asteroids, int asteroid) {
    int asteroidIndex;
    for (asteroidIndex = asteroid; asteroidIndex < num_of_asteroids; asteroidIndex++) {
        asteriods[asteroidIndex] = asteriods[asteroidIndex + 1];
    }
    num_of_asteroids--;
}

void destroyShot(int shot) {
    int shotIndex;
    for (shotIndex = shot; shotIndex < shotsCounter; shotIndex++) {
        shots[shotIndex] = shots[shotIndex + 1];
    }
    shotsCounter--;
}

void destroyShip(Ship spaceship) { // Destroyed ship particle effect and gameover screen
    int particleAngle;
    if (!stats.exploding) {
        PlaySoundA((LPCSTR)"assets//audio//shipExp.wav", NULL, SND_FILENAME | SND_ASYNC);
        stats.gameOverTime = clock();
        stats.exploding = true;
        for (particleAngle = 0; particleAngle < 360; particleAngle += 25) {
            Vector2 particlePos = rotateVector(0, -1, particleAngle);
            Shot particle = createShot(spaceship.entity.pos.x, spaceship.entity.pos.y, particlePos.x, particlePos.y, 500);
            shots[++shotsCounter] = particle;
        }
    }
}

bool pointInsidePolygon(Vector2 polygon[], Vector2 point, int len) {
    bool isInside = false;
    int i, j;
    for (i = 0, j = len - 1; i < len; j = i++) {
        if (((polygon[i].y >= point.y) != (polygon[j].y >= point.y)) &&
            (point.x <= (polygon[j].x - polygon[i].x) * (point.y - polygon[i].y) / (polygon[j].y - polygon[i].y) + polygon[i].x)) {
            isInside = !isInside;
        }
    }
    return isInside;
}

void calculateAsteroidPhysics (Ship spaceship, Asteroid asteroids[], int num_of_asteroids) { // Apply asteroid physics
    int i, j;
    for (i = 0; i < num_of_asteroids; i++) {
        asteroids[i].entity.pos.x += asteroids[i].entity.velocity.y;
        asteroids[i].entity.pos.y += asteroids[i].entity.velocity.y;
        asteroids[i].angle += asteroids[i].vangle;

        if (asteroids[i].entity.pos.x < 0) asteroids[i].entity.pos.x += WIDTH;
        if (asteroids[i].entity.pos.x > WIDTH - 20) asteroids[i].entity.pos.x -= WIDTH;
        if (asteroids[i].entity.pos.y < 0) asteroids[i].entity.pos.y += HEIGHT;
        if (asteroids[i].entity.pos.y > HEIGHT - 20) asteroids[i].entity.pos.y -= HEIGHT;

        for (j = 0; j < 8; j++) {
            Vector2 AsteroidEdge = rotateVector(asteroids[i].model[j].x, asteroids[i].model[j].y, asteroids[i].angle);
            AsteroidEdge = { AsteroidEdge.x + asteroids[i].entity.pos.x, AsteroidEdge.y + asteroids[i].entity.pos.y };
            asteroids[i].edges[j] = AsteroidEdge;
            if (pointInsidePolygon(spaceship.edges, asteroids[i].edges[j], 5) && !stats.exploding) { // Check if Edge of asteroid is inside the ship
                CC212VSGL::drawText((WIDTH - 100) / 2, HEIGHT - 50, "Asteroid collision With Ship"); // DEBUG, will be removed
                destroyShip(spaceship);
                // stats.alive = false;
            }
        }

        for (j = 0; j < 5; j++) { // TODO: Add ship explosion
            if (pointInsidePolygon(asteroids[i].edges, spaceship.edges[j], 8) && !stats.exploding) { // Check if edge of ship is inside the asteroid
                destroyShip(spaceship);
                // stats.alive = false;
                CC212VSGL::drawText((WIDTH - 100) / 2, HEIGHT - 50, "Ship Collision With Asteroid"); // DEBUG, will be removed
            }
        }
    }
}

void renderShots(Asteroid asteroids[], int &num_of_asteroids) { // Renders shots and apply physics.
    int shotIndex, asteroidIndex; 
    bool shotDestroyed = false;
    for (shotIndex = 0; shotIndex < shotsCounter; shotIndex++) {
        SYSTEMTIME currentTime;
        GetSystemTime(&currentTime);
        if (shots[shotIndex].birth + shots[shotIndex].lifespan > currentTime.wMilliseconds) {
            shotDestroyed = false;
            shots[shotIndex].entity.pos.x += shots[shotIndex].entity.velocity.x;
            shots[shotIndex].entity.pos.y += shots[shotIndex].entity.velocity.y;
            for (asteroidIndex = 0; asteroidIndex < num_of_asteroids; asteroidIndex++) {
                if (pointInsidePolygon(asteroids[asteroidIndex].edges, shots[shotIndex].entity.pos, 8)) { // Collision check between asteroid and shot
                    Vector2 shotVelocity = { shots[shotIndex].entity.velocity.x, shots[shotIndex].entity.velocity.y };
                    destroyShot(shotIndex);
                    shotDestroyed = true;
                    if (asteroids[asteroidIndex].health > 10) { // Still strong
                        // CC212VSGL::drawText((WIDTH - 100) / 2, HEIGHT - 70, "Big asteroid strong");
                        asteroids[asteroidIndex].health -= 1;
                        asteroids[asteroidIndex].entity.velocity.x += shotVelocity.x * 0.02; // + 0.5 - random_float(0.1, 0.5);
                        asteroids[asteroidIndex].entity.velocity.y += shotVelocity.y * 0.02; // + 0.5 - random_float(0.1, 0.5);
                    }
                    else { // Weak destroyed
                        if (asteroids[asteroidIndex].size / 2 > 10) { // If big asteroid split it.
                            // CC212VSGL::drawText((WIDTH - 100) / 2, HEIGHT - 70, "Big asteroid destroyed");
                            int i;
                            for (i = 0; i < 2; i++) {
                                asteroids[num_of_asteroids++] = createAsteroid(0.0, 0.0, asteroids[asteroidIndex].size / 2, asteroids[asteroidIndex].entity.pos.x, asteroids[asteroidIndex].entity.pos.y);
                            }
                        }
                        stats.score += 10;
                        explosions[num_of_explosions++] = createExplosion(asteroids[asteroidIndex].entity.pos.x, asteroids[asteroidIndex].entity.pos.y, asteroids[asteroidIndex].size / EXPLOSION_DEFAULT_SIZE);
                        destroyAsteroid(asteroids, num_of_asteroids, asteroidIndex); // Destroy that weak asteroid
                        if (!stats.exploding) PlaySoundA((LPCSTR)"assets//audio//hit.wav", NULL, SND_FILENAME | SND_ASYNC);
                        // CC212VSGL::drawText((WIDTH - 100) / 2, HEIGHT - 85, "Removing that asteroid");
                    }
                }
            }
            if (!shotDestroyed) {
                if (shots[shotIndex].entity.pos.x < 0 || shots[shotIndex].entity.pos.y < 0 || shots[shotIndex].entity.pos.x > WIDTH || shots[shotIndex].entity.pos.y > HEIGHT) {
                    destroyShot(shotIndex);
                    shotDestroyed = true;
                }
                if (!shotDestroyed) CC212VSGL::drawLine(shots[shotIndex].entity.pos.x, shots[shotIndex].entity.pos.y, shots[shotIndex].entity.pos.x - shots[shotIndex].dx * 8, shots[shotIndex].entity.pos.y - shots[shotIndex].dy * 8);
            }
        } else {
            destroyShot(shotIndex);
        }
    }
}

void renderAsteroids(Asteroid asteroids[], int num_of_asteroids) { // Renders all the asteroids through specified points
    Vector2 firstPoint;
    int modelIndex, asteroidIndex;
    for (asteroidIndex = 0; asteroidIndex < num_of_asteroids; asteroidIndex++) {
        for (modelIndex = 0; modelIndex < 8; modelIndex++) {
            Vector2 point = rotateVector(asteroids[asteroidIndex].model[modelIndex].x, asteroids[asteroidIndex].model[modelIndex].y, asteroids[asteroidIndex].angle);
            point = { point.x + asteroids[asteroidIndex].entity.pos.x, point.y + asteroids[asteroidIndex].entity.pos.y };
            if (modelIndex == 0) {
                firstPoint = point;
            }

            if (modelIndex == 7) {
                CC212VSGL::drawLine(point.x, point.y, firstPoint.x, firstPoint.y); // Last point with first point
            }
            else {
                Vector2 nextPoint = rotateVector(asteroids[asteroidIndex].model[modelIndex + 1].x, asteroids[asteroidIndex].model[modelIndex + 1].y, asteroids[asteroidIndex].angle);
                nextPoint = { nextPoint.x + asteroids[asteroidIndex].entity.pos.x, nextPoint.y + asteroids[asteroidIndex].entity.pos.y };
                CC212VSGL::drawLine(point.x, point.y, nextPoint.x, nextPoint.y); // Current point with next point
            }
        }
    }
}

void generateAsteroids(Asteroid asteroids[], int &asteroidsCreated, int n, float marginX = 0.0, float marginY = 0.0, int size = (rand() % (80 - 40)) + 80) { // Generates asteroids with in a specified margin
    int i, start = asteroidsCreated;
    for (i = start; i < n; i++) {
        asteroids[i] = createAsteroid(marginX, marginY, size);
        asteroidsCreated++;
    }
}

void resetStats() { // reset everything for restart.
    stats.exploding = false;
    stats.gameOver = false;
    stats.gameOverTime = 0;
    stats.alive = true;
    if (stats.score > stats.previousHighScore) stats.previousHighScore = stats.score;
    stats.score = 0;
    stats.shots = 0;
    stats.level = 0;
}

void spawnShip(Ship &spaceship) {
    // Spawns the ship into the center.
    spaceship.entity.velocity.x = 0;
    spaceship.entity.velocity.y = 0;
    spaceship.entity.pos.x = WIDTH / 2;
    spaceship.entity.pos.y = HEIGHT / 2;
    resetStats();
}

void intro(Ship &spaceship) { // Intro of game.
    Asteroid asteroids[100];
    bool keys[256];
    int asteroidsCreated = 0;
    int introImg = CC212VSGL::loadImage("assets\\img\\icon.png");

    mciSendStringA("open assets\\audio\\intro.mp3 type mpegvideo", NULL, 0, 0);
    mciSendStringA("play assets\\audio\\intro.mp3 repeat", NULL, 0, 0);
    generateAsteroids(asteroids, asteroidsCreated, 40, 0, 0, 60); // Generates Asteroids

    do {
        ReadKeyboard(keys);
        CC212VSGL::beginDraw();
        CC212VSGL::setDrawingColor(WHITE);
        renderAsteroids(asteroids, asteroidsCreated);
        calculateAsteroidPhysics(spaceship, asteroids, asteroidsCreated);
        CC212VSGL::drawImage(introImg, (WIDTH / 4) - 30, 50, 0);
        CC212VSGL::resizeImage(introImg, WIDTH / 6, HEIGHT / 6);
        CC212VSGL::setFontSizeAndBoldness(40, 5);
        CC212VSGL::drawText((WIDTH / 4) + 100, 70, "Astronautilus");
        CC212VSGL::setFontSizeAndBoldness(25, 10);
        CC212VSGL::drawText((WIDTH / 4) + 80, (HEIGHT / 2), "Press Space to start.");
        CC212VSGL::drawRectangle(0, 0, WIDTH, HEIGHT);
        CC212VSGL::endDraw();
        if (keys[' ']) { // Start the game when space is pressed.
            mciSendStringA("stop assets\\audio\\intro.mp3 ", NULL, 0, 0);
            stats.intro = false;
            spawnShip(spaceship);
        }
    } while (stats.intro);
}

void renderShip(Ship spaceship) { // Renders the ship's structure through specified points.
    Vector2 firstPoint;
    int i = 0;
    
    for (i = 0; i < 5; i++) {
        Vector2 point = { spaceship.model[i].x, spaceship.model[i].y };
        point = rotateVector(point.x, point.y, spaceship.angle);
        point = { point.x + spaceship.entity.pos.x, point.y + spaceship.entity.pos.y };
        if (i == 0) {
            firstPoint = point;
        } 
        
        if (i == 4) {
            CC212VSGL::drawLine(point.x, point.y, firstPoint.x, firstPoint.y);
        }
        else {
            Vector2 nextPoint = rotateVector(spaceship.model[i + 1].x, spaceship.model[i + 1].y, spaceship.angle);
            nextPoint = { nextPoint.x + spaceship.entity.pos.x, nextPoint.y + spaceship.entity.pos.y };
            CC212VSGL::drawLine(point.x, point.y, nextPoint.x, nextPoint.y);
        }
    }
}

int main () {
    Ship spaceship;
    Asteroid asteroids[100];
    // Explosion explosions[100];
    SYSTEMTIME time;
    GetSystemTime(&time);
    int frames = 0, i, num_of_asteroids = 0;
    int lastCalledTime = time.wSecond, lastShot = time.wSecond, timeSinceStart;
    bool keys[256];
  
    // Start setup
    CC212VSGL::setup();
    CC212VSGL::setFullScreenMode();
    CC212VSGL::hideCursor();
    intro(spaceship);
    generateAsteroids(asteroids, num_of_asteroids, ASTEROID_DEFAULT_RATE, spaceship.entity.pos.x, spaceship.entity.pos.y);
    
    while (true) {
        SYSTEMTIME time2;
        Vector2 currentVector = rotateVector(0, -1, spaceship.angle);
        lastShot++;

        CC212VSGL::beginDraw();
        CC212VSGL::resetFontSize();
        CC212VSGL::setDrawingColor(WHITE);
        CC212VSGL::drawRectangle(0, 0, WIDTH, HEIGHT); // Boundary

        ReadKeyboard(keys);
        if (stats.alive) {
            if (num_of_asteroids <= 0) {
                stats.level += 1;
                // generateAsteroids(asteroids, num_of_asteroids, stats.level + ASTEROID_DEFAULT_RATE);
            }
            if (stats.exploding) { // Ship exploding, leave a 2 seconds gap before showing game over screen
                if ((clock() - stats.gameOverTime) / (double)CLOCKS_PER_SEC >= 1.5) {
                    stats.alive = false; // shows the game over screen.
                }
            }
            if (!stats.exploding) renderShip(spaceship); // Render ship if not exploding
            renderAsteroids(asteroids, num_of_asteroids); // Renders all current asteroids
            calculateAsteroidPhysics(spaceship, asteroids, num_of_asteroids); // Apply their physics when rendered
            renderShots(asteroids, num_of_asteroids); // Render/Apply physics current active shots

            // Frames calculation (Not limited)
            GetSystemTime(&time2);
            int secondsNow = time2.wSecond;
            if (lastCalledTime != secondsNow) { // Calculate FPS through time (s)
                consoleFPS = frames;
                frames = 0;
            }
            else {
                frames += 1;
            }
            lastCalledTime = secondsNow;

            // Render Explosions
            int explosionIndex, explosionRate;
            for (explosionIndex = 0; explosionIndex < num_of_explosions; explosionIndex++) {
                explosions[explosionIndex].age += 1000 / ((int)consoleFPS + 1);
                if (explosions[explosionIndex].age < EXPLOSION_DEFAULT_AGE) {
                    for (explosionRate = 0; explosionRate < 5; explosionRate++) {
                        explosions[explosionIndex].size += explosionRate;
                        CC212VSGL::drawCircle(explosions[explosionIndex].entity.pos.x, explosions[explosionIndex].entity.pos.y, explosions[explosionIndex].size / EXPLOSION_DEFAULT_SIZE);
                    }
                }
            }

            // Enable controls when not exploding
            if (!stats.exploding) {
                // Rotation based on FPS smoothness
                if (keys['A']) spaceship.angle -= SHIP_ROT_SPEED * consoleFPS / 600;
                else if (keys['D']) spaceship.angle += SHIP_ROT_SPEED * consoleFPS / 600;

                // Ship movement
                if (keys['W']) {
                    spaceship.thrust += consoleFPS / (60.0 * 1000.0); // Increases the smooth of motion
                }
                else {
                    spaceship.thrust = spaceship.thrust * 0.9; // Lower the speed of thrust over time
                }
                if (keys['S']) {
                    // air brake
                    spaceship.entity.velocity.x = spaceship.entity.velocity.x * 0.95;
                    spaceship.entity.velocity.y = spaceship.entity.velocity.y * 0.95;
                    spaceship.airbrake = true;
                }
                else {
                    spaceship.airbrake = false;
                }

                float currentThrustResultVelocity = sqrt(fabs(pow(spaceship.entity.velocity.x + currentVector.x * spaceship.thrust, 2)) + fabs(pow(spaceship.entity.velocity.y + currentVector.y * spaceship.thrust, 2)));

                if (currentThrustResultVelocity < SHIP_MAX_VELOCITY) {
                    // if we aim against our velocity vector
                    spaceship.entity.velocity.x += (currentVector.x * spaceship.thrust);
                    spaceship.entity.velocity.y += (currentVector.y * spaceship.thrust);
                }

                spaceship.entity.velocityTotal = sqrt(fabs(spaceship.entity.velocity.x * spaceship.entity.velocity.x) + fabs(spaceship.entity.velocity.y * spaceship.entity.velocity.y));

                // Ship movement update
                spaceship.entity.pos.x += spaceship.entity.velocity.x;
                spaceship.entity.pos.y += spaceship.entity.velocity.y;

                // Ship movement boundaries
                if (spaceship.entity.pos.x < 0) spaceship.entity.pos.x += WIDTH;
                if (spaceship.entity.pos.x > WIDTH) spaceship.entity.pos.x -= WIDTH;
                if (spaceship.entity.pos.y < 0) spaceship.entity.pos.y += HEIGHT;
                if (spaceship.entity.pos.y > HEIGHT) spaceship.entity.pos.y -= HEIGHT;

                // Collisions update
                for (i = 0; i < 5; i++) {
                    Vector2 fixedShipEdge = rotateVector(spaceship.model[i].x, spaceship.model[i].y, spaceship.angle);
                    fixedShipEdge = { fixedShipEdge.x + spaceship.entity.pos.x, fixedShipEdge.y + spaceship.entity.pos.y };
                    spaceship.edges[i] = fixedShipEdge;
                }

                if (keys[' '] && lastShot >= 10) { // Shots
                    if (shotsCounter < 1000) {
                        Shot newShot = createShot(spaceship.edges[0].x, spaceship.edges[0].y, spaceship.entity.velocity.x + currentVector.x * 8 + 0.5, spaceship.entity.velocity.y + currentVector.y * 8 + 0.5);
                        shots[shotsCounter] = newShot;
                        shotsCounter++;
                        stats.shots++;
                        lastShot = 0;
                    }
                }
            }
 
            // Labels HUD
            char fpsLabel[32] = {};
            char velocityLabel[32] = {};
            char thrustLabel[32] = {};
            char scoreLabel[64] = {};
            char highScoreLabel[64] = {};
            char shotsLabel[64] = {};
            sprintf_s(fpsLabel, "FPS: %d", (int)consoleFPS);
            sprintf_s(velocityLabel, "Velocity: %.1f", spaceship.entity.velocityTotal);
            sprintf_s(shotsLabel, "Shots: %d", stats.shots);
            sprintf_s(scoreLabel, "Score: %d", stats.score);
            sprintf_s(highScoreLabel, "High score: %d", stats.previousHighScore);
            // sprintf_s(thrustLabel, "Thrust: %.01f", spaceship.thrust);
            // sprintf_s(vectX, "X: %f", currentVector.x);
            // sprintf_s(vectY, "Y: %f", currentVector.y);
            // sprintf_s(cThrust, "thrustResVel: %f", currentThrustResultVelocity);

            CC212VSGL::drawText(10, 10, fpsLabel);
            CC212VSGL::drawText(10, 40, velocityLabel);

            CC212VSGL::setFontSizeAndBoldness(20, 5);
            CC212VSGL::drawText(WIDTH - 120, 0, scoreLabel);
            CC212VSGL::drawText(WIDTH - 120, 20, highScoreLabel);
            CC212VSGL::drawText(WIDTH - 120, 40, shotsLabel);

           
            // CC212VSGL::drawText(10, 55, thrustLabel);
            // CC212VSGL::drawText(550, 10, vectX);
            // CC212VSGL::drawText(700, 10, vectY);
            // CC212VSGL::drawText(100, 25, cThrust);

            if (!stats.exploding && spaceship.airbrake && spaceship.entity.velocityTotal > 1) {
                CC212VSGL::setDrawingColor(RED);
                CC212VSGL::drawText((WIDTH - 100) / 2, HEIGHT - 20, "Airbrake Active");
            }
        }
        else if (!stats.intro) { // If not intro and ship destroyed, display gameover screen.
            char scoreLabel[64] = {};
            char highScoreLabel[64] = {};
            sprintf_s(scoreLabel, "Score: %d", stats.score); 
            sprintf_s(highScoreLabel, "High score: %d", stats.previousHighScore);
            if (!stats.gameOver) {
                stats.gameOver = true;
                PlaySoundA((LPCSTR)"assets//audio//fail.wav", NULL, SND_FILENAME | SND_ASYNC);
            }
            CC212VSGL::setDrawingColor(RED);
            CC212VSGL::setFontSizeAndBoldness(50, 10);
            CC212VSGL::drawText((WIDTH / 2) - 120, (HEIGHT / 2) - 100, "GAME OVER");
            CC212VSGL::setDrawingColor(GREEN);
            CC212VSGL::setFontSizeAndBoldness(35, 5);
            CC212VSGL::drawText(WIDTH / 2 - 70, (HEIGHT / 2) - 20, scoreLabel);
            CC212VSGL::drawText(WIDTH / 2 - 70, (HEIGHT / 2) + 30, highScoreLabel);
            CC212VSGL::setDrawingColor(WHITE);
            CC212VSGL::setFontSizeAndBoldness(38, 10);
            CC212VSGL::drawText(WIDTH / 2 - 140, (HEIGHT / 2) + 80, "Press R to Restart");
            if (keys['R']) { // Restart the game.
                num_of_asteroids = 0;
                num_of_explosions = 0;
                shotsCounter = 0;
                generateAsteroids(asteroids, num_of_asteroids, ASTEROID_DEFAULT_RATE, (float)WIDTH / 2, (float)HEIGHT / 2);
                spawnShip(spaceship);
            }
        }
        CC212VSGL::endDraw();
    }

    return 0;

}