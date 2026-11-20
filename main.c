/*
 * CITY LIFE 3D - Original open-world mobile game prototype
 * Language: C99 | Renderer: raylib 5.5 | Android host: raymob template
 * This is an original project and is not affiliated with Rockstar Games or GTA.
 */
#include "raymob.h"
#include "rlgl.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#define MAX_BUILDINGS 160
#define CITY_LIMIT 128.0f
#define PI_F 3.14159265358979323846f
#define DEG_PER_RAD 57.29577951308232f

#define COLOR_GRASS   ((Color){ 73, 132, 78, 255 })
#define COLOR_ROAD    ((Color){ 48, 52, 59, 255 })
#define COLOR_SIDE    ((Color){ 126, 128, 126, 255 })
#define COLOR_WINDOW  ((Color){ 126, 196, 222, 255 })
#define COLOR_CAR     ((Color){ 197, 48, 54, 255 })
#define COLOR_TIRE    ((Color){ 25, 27, 31, 255 })
#define COLOR_GOLD    ((Color){ 252, 196, 62, 255 })
#define COLOR_HUD     ((Color){ 12, 18, 28, 220 })

typedef struct {
    Vector3 position;
    float height;
    float width;
    float depth;
    Color color;
} Building;

typedef struct {
    Vector3 position;
    float yaw;
    float speed;
} Vehicle;

typedef struct {
    float x;
    float y;
} MoveInput;

static float ClampF(float value, float minimum, float maximum) {
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

static float Distance2D(float x1, float z1, float x2, float z2) {
    float dx = x1 - x2;
    float dz = z1 - z2;
    return sqrtf(dx * dx + dz * dz);
}

static float NormalizeAngle(float angle) {
    while (angle > PI_F) angle -= 2.0f * PI_F;
    while (angle < -PI_F) angle += 2.0f * PI_F;
    return angle;
}

static bool PointInRect(Vector2 point, Rectangle rect) {
    return point.x >= rect.x && point.x <= rect.x + rect.width &&
           point.y >= rect.y && point.y <= rect.y + rect.height;
}

static bool TouchInRect(Rectangle rect) {
    int count = GetTouchPointCount();
    for (int i = 0; i < count; i++) {
        if (PointInRect(GetTouchPosition(i), rect)) return true;
    }
    return false;
}

static void BuildCity(Building *buildings, int *count) {
    *count = 0;
    for (int blockX = -3; blockX < 3; blockX++) {
        for (int blockZ = -3; blockZ < 3; blockZ++) {
            float centerX = (float)blockX * 40.0f + 20.0f;
            float centerZ = (float)blockZ * 40.0f + 20.0f;

            for (int sideX = -1; sideX <= 1; sideX += 2) {
                for (int sideZ = -1; sideZ <= 1; sideZ += 2) {
                    if (*count >= MAX_BUILDINGS) return;

                    int seed = (blockX + 5) * 37 + (blockZ + 5) * 19 +
                               (sideX + 2) * 11 + (sideZ + 2) * 7;
                    int variation = seed < 0 ? -seed : seed;
                    Building building;
                    building.position = (Vector3){
                        centerX + (float)sideX * 8.0f,
                        0.0f,
                        centerZ + (float)sideZ * 8.0f
                    };
                    building.height = 9.0f + (float)(variation % 6) * 2.6f;
                    building.width = 12.0f;
                    building.depth = 12.0f;
                    building.color = (Color){
                        (unsigned char)(126 + (variation % 54)),
                        (unsigned char)(119 + ((variation * 3) % 58)),
                        (unsigned char)(112 + ((variation * 5) % 65)),
                        255
                    };
                    buildings[*count] = building;
                    (*count)++;
                }
            }
        }
    }
}

static bool HitsBuilding(float x, float z, float radius,
                         const Building *buildings, int count) {
    for (int i = 0; i < count; i++) {
        float minX = buildings[i].position.x - buildings[i].width * 0.5f;
        float maxX = buildings[i].position.x + buildings[i].width * 0.5f;
        float minZ = buildings[i].position.z - buildings[i].depth * 0.5f;
        float maxZ = buildings[i].position.z + buildings[i].depth * 0.5f;
        float nearestX = ClampF(x, minX, maxX);
        float nearestZ = ClampF(z, minZ, maxZ);
        if (Distance2D(x, z, nearestX, nearestZ) < radius) return true;
    }
    return false;
}

static void MoveWithCollision(Vector3 *position, float dx, float dz,
                              float radius, const Building *buildings,
                              int buildingCount) {
    float nextX = ClampF(position->x + dx, -CITY_LIMIT, CITY_LIMIT);
    if (!HitsBuilding(nextX, position->z, radius, buildings, buildingCount)) {
        position->x = nextX;
    }

    float nextZ = ClampF(position->z + dz, -CITY_LIMIT, CITY_LIMIT);
    if (!HitsBuilding(position->x, nextZ, radius, buildings, buildingCount)) {
        position->z = nextZ;
    }
}

static void DrawRoads(void) {
    for (int road = -120; road <= 120; road += 40) {
        // Main road strips and sidewalks
        DrawCube((Vector3){(float)road, 0.02f, 0.0f}, 10.0f, 0.08f, 280.0f, COLOR_ROAD);
        DrawCube((Vector3){0.0f, 0.02f, (float)road}, 280.0f, 0.08f, 10.0f, COLOR_ROAD);
        DrawCube((Vector3){(float)road - 6.0f, 0.035f, 0.0f}, 1.1f, 0.10f, 280.0f, COLOR_SIDE);
        DrawCube((Vector3){(float)road + 6.0f, 0.035f, 0.0f}, 1.1f, 0.10f, 280.0f, COLOR_SIDE);
        DrawCube((Vector3){0.0f, 0.035f, (float)road - 6.0f}, 280.0f, 0.10f, 1.1f, COLOR_SIDE);
        DrawCube((Vector3){0.0f, 0.035f, (float)road + 6.0f}, 280.0f, 0.10f, 1.1f, COLOR_SIDE);

        for (int mark = -130; mark <= 130; mark += 14) {
            DrawCube((Vector3){(float)road, 0.085f, (float)mark},
                     0.20f, 0.025f, 5.0f, COLOR_GOLD);
            DrawCube((Vector3){(float)mark, 0.085f, (float)road},
                     5.0f, 0.025f, 0.20f, COLOR_GOLD);
        }
    }
}

static void DrawBuildings(const Building *buildings, int count) {
    for (int i = 0; i < count; i++) {
        const Building *b = &buildings[i];
        Vector3 center = { b->position.x, b->height * 0.5f, b->position.z };
        DrawCube(center, b->width, b->height, b->depth, b->color);
        DrawCubeWires(center, b->width, b->height, b->depth,
                      (Color){55, 60, 69, 255});

        // Two large windows on the side facing the nearest street.
        float windowZ = b->position.z - b->depth * 0.5f - 0.055f;
        float windowY = ClampF(b->height * 0.57f, 3.2f, b->height - 1.3f);
        for (int side = -1; side <= 1; side += 2) {
            DrawCube((Vector3){b->position.x + side * 2.7f, windowY, windowZ},
                     1.55f, 1.55f, 0.09f, COLOR_WINDOW);
            DrawCubeWires((Vector3){b->position.x + side * 2.7f, windowY, windowZ},
                          1.55f, 1.55f, 0.09f, (Color){35, 48, 60, 255});
        }
        DrawCube((Vector3){b->position.x, b->height + 0.18f, b->position.z},
                 b->width + 0.35f, 0.35f, b->depth + 0.35f,
                 (Color){75, 78, 84, 255});
    }
}

static void DrawVehicle(const Vehicle *car) {
    rlPushMatrix();
    rlTranslatef(car->position.x, car->position.y, car->position.z);
    rlRotatef(car->yaw * DEG_PER_RAD, 0.0f, 1.0f, 0.0f);

    // Chassis, cabin, bumpers, lights and four wheels
    DrawCube((Vector3){0.0f, 0.85f, 0.0f}, 2.15f, 0.80f, 4.15f, COLOR_CAR);
    DrawCubeWires((Vector3){0.0f, 0.85f, 0.0f}, 2.15f, 0.80f, 4.15f,
                  (Color){75, 24, 29, 255});
    DrawCube((Vector3){0.0f, 1.48f, -0.25f}, 1.72f, 0.72f, 2.15f,
             (Color){82, 143, 171, 255});
    DrawCube((Vector3){0.0f, 0.38f, -2.10f}, 2.0f, 0.22f, 0.20f, DARKGRAY);
    DrawCube((Vector3){0.0f, 0.38f,  2.10f}, 2.0f, 0.22f, 0.20f, DARKGRAY);
    DrawCube((Vector3){-0.64f, 0.95f, -2.09f}, 0.35f, 0.22f, 0.08f, RAYWHITE);
    DrawCube((Vector3){ 0.64f, 0.95f, -2.09f}, 0.35f, 0.22f, 0.08f, RAYWHITE);

    DrawSphere((Vector3){-1.08f, 0.53f, -1.30f}, 0.43f, COLOR_TIRE);
    DrawSphere((Vector3){ 1.08f, 0.53f, -1.30f}, 0.43f, COLOR_TIRE);
    DrawSphere((Vector3){-1.08f, 0.53f,  1.30f}, 0.43f, COLOR_TIRE);
    DrawSphere((Vector3){ 1.08f, 0.53f,  1.30f}, 0.43f, COLOR_TIRE);
    rlPopMatrix();
}

static void DrawPlayer(Vector3 position, float yaw, float walkPhase) {
    rlPushMatrix();
    rlTranslatef(position.x, position.y, position.z);
    rlRotatef(yaw * DEG_PER_RAD, 0.0f, 1.0f, 0.0f);

    float bob = sinf(walkPhase) * 0.045f;
    DrawCube((Vector3){0.0f, 1.02f + bob, 0.0f}, 0.78f, 1.05f, 0.48f,
             (Color){36, 91, 174, 255});
    DrawSphere((Vector3){0.0f, 1.78f + bob, 0.0f}, 0.34f,
               (Color){205, 164, 128, 255});
    DrawCube((Vector3){-0.49f, 1.02f + bob, 0.0f}, 0.22f, 0.86f, 0.24f,
             (Color){32, 75, 140, 255});
    DrawCube((Vector3){ 0.49f, 1.02f + bob, 0.0f}, 0.22f, 0.86f, 0.24f,
             (Color){32, 75, 140, 255});
    DrawCube((Vector3){-0.22f, 0.38f + bob, 0.0f}, 0.26f, 0.68f, 0.30f, DARKBLUE);
    DrawCube((Vector3){ 0.22f, 0.38f - bob, 0.0f}, 0.26f, 0.68f, 0.30f, DARKBLUE);
    rlPopMatrix();
}

static void DrawMissionMarker(Vector3 marker, int number) {
    DrawCylinder((Vector3){marker.x, 0.85f, marker.z}, 1.0f, 1.0f,
                 1.7f, 20, Fade(GREEN, 0.72f));
    DrawCylinderWires((Vector3){marker.x, 0.85f, marker.z}, 1.0f, 1.0f,
                      1.7f, 20, RAYWHITE);
    DrawSphere((Vector3){marker.x, 2.0f, marker.z}, 0.48f, COLOR_GOLD);
    (void)number;
}

static void DrawButton(Rectangle rect, const char *label, Color color) {
    DrawRectangleRounded(rect, 0.22f, 8, Fade(color, 0.80f));
    DrawRectangleRoundedLines(rect, 0.22f, 8, Fade(RAYWHITE, 0.85f));
    int textWidth = MeasureText(label, 18);
    DrawText(label, (int)(rect.x + (rect.width - (float)textWidth) * 0.5f),
             (int)(rect.y + (rect.height - 18.0f) * 0.5f), 18, RAYWHITE);
}

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(1280, 720, "CITY LIFE 3D");
    SetTargetFPS(60);

    Building buildings[MAX_BUILDINGS];
    int buildingCount = 0;
    BuildCity(buildings, &buildingCount);

    Vector3 player = {3.2f, 0.0f, 3.2f};
    float playerYaw = 0.0f;
    float walkPhase = 0.0f;
    float jumpVelocity = 0.0f;
    bool grounded = true;
    bool inCar = false;
    bool actionWasDown = false;
    int cash = 0;
    int missionIndex = 0;

    Vehicle car = {{0.0f, 0.0f, 0.0f}, 0.0f, 0.0f};
    const Vector3 missionPoints[3] = {
        {40.0f, 0.0f, -40.0f},
        {-40.0f, 0.0f, 40.0f},
        {80.0f, 0.0f, 0.0f}
    };

    Camera3D camera = {0};
    camera.position = (Vector3){player.x, 6.0f, player.z + 10.0f};
    camera.target = (Vector3){player.x, 1.0f, player.z};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    while (!WindowShouldClose()) {
        float dt = ClampF(GetFrameTime(), 0.0f, 0.05f);
        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();

        Rectangle actionRect = {(float)screenW - 116.0f, (float)screenH - 258.0f, 96.0f, 58.0f};
        Rectangle gasRect = {(float)screenW - 112.0f, (float)screenH - 142.0f, 94.0f, 82.0f};
        Rectangle brakeRect = {(float)screenW - 218.0f, (float)screenH - 132.0f, 92.0f, 72.0f};

        MoveInput input = {0.0f, 0.0f};
        bool actionDown = IsKeyDown(KEY_E) || TouchInRect(actionRect);
        bool actionPressed = actionDown && !actionWasDown;
        actionWasDown = actionDown;
        bool gasDown = IsKeyDown(KEY_UP) || IsKeyDown(KEY_W) || TouchInRect(gasRect);
        bool brakeDown = IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S) || TouchInRect(brakeRect);

        // Keyboard movement (also useful for desktop debugging).
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) input.x -= 1.0f;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) input.x += 1.0f;
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) input.y += 1.0f;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) input.y -= 1.0f;

        // Virtual analogue stick on the lower-left side of the screen.
        float stickX = (float)screenW * 0.17f;
        float stickY = (float)screenH * 0.79f;
        for (int touch = 0; touch < GetTouchPointCount(); touch++) {
            Vector2 p = GetTouchPosition(touch);
            if (p.x < (float)screenW * 0.48f && p.y > (float)screenH * 0.42f) {
                float dx = p.x - stickX;
                float dy = p.y - stickY;
                float length = sqrtf(dx * dx + dy * dy);
                if (length > 8.0f) {
                    float scale = length > 62.0f ? 62.0f / length : 1.0f;
                    input.x = ClampF((dx * scale) / 62.0f, -1.0f, 1.0f);
                    input.y = ClampF((-dy * scale) / 62.0f, -1.0f, 1.0f);
                }
            }
        }
        input.x = ClampF(input.x, -1.0f, 1.0f);
        input.y = ClampF(input.y, -1.0f, 1.0f);

        if (!inCar) {
            if (actionPressed) {
                if (Distance2D(player.x, player.z, car.position.x, car.position.z) < 5.5f) {
                    inCar = true;
                    car.position = player;
                    car.position.y = 0.0f;
                    car.yaw = playerYaw;
                } else if (grounded) {
                    jumpVelocity = 6.6f;
                    grounded = false;
                }
            }

            playerYaw = NormalizeAngle(playerYaw + input.x * 2.25f * dt);
            float forwardAmount = input.y;
            float dx = sinf(playerYaw) * forwardAmount * 6.0f * dt;
            float dz = -cosf(playerYaw) * forwardAmount * 6.0f * dt;
            MoveWithCollision(&player, dx, dz, 0.62f, buildings, buildingCount);

            if (fabsf(input.y) > 0.05f) walkPhase += dt * 10.0f;
            jumpVelocity -= 18.0f * dt;
            player.y += jumpVelocity * dt;
            if (player.y < 0.0f) {
                player.y = 0.0f;
                jumpVelocity = 0.0f;
                grounded = true;
            }
        } else {
            if (actionPressed) {
                inCar = false;
                car.speed = 0.0f;
                player.y = 0.0f;
                playerYaw = car.yaw;
                player.x = car.position.x + cosf(car.yaw) * 3.0f;
                player.z = car.position.z + sinf(car.yaw) * 3.0f;
                if (HitsBuilding(player.x, player.z, 0.65f, buildings, buildingCount)) {
                    player.x = car.position.x - cosf(car.yaw) * 3.0f;
                    player.z = car.position.z - sinf(car.yaw) * 3.0f;
                }
            } else {
                float throttle = input.y;
                if (gasDown) throttle = 1.0f;
                if (brakeDown) throttle = -0.25f;
                car.speed += throttle * 14.0f * dt;
                if (!gasDown && !brakeDown && fabsf(input.y) < 0.05f) {
                    car.speed *= (1.0f - 1.15f * dt);
                }
                if (brakeDown) {
                    if (car.speed > 0.0f) car.speed = fmaxf(0.0f, car.speed - 24.0f * dt);
                    else car.speed = fminf(0.0f, car.speed + 24.0f * dt);
                }
                car.speed = ClampF(car.speed, -8.0f, 22.0f);
                float steerFactor = ClampF(fabsf(car.speed) / 4.0f, 0.25f, 1.25f);
                car.yaw = NormalizeAngle(car.yaw + input.x * 1.75f * steerFactor * dt *
                                         (car.speed < 0.0f ? -1.0f : 1.0f));
                float carDX = sinf(car.yaw) * car.speed * dt;
                float carDZ = -cosf(car.yaw) * car.speed * dt;
                float oldX = car.position.x;
                float oldZ = car.position.z;
                MoveWithCollision(&car.position, carDX, carDZ, 1.65f,
                                  buildings, buildingCount);
                if (fabsf(car.position.x - (oldX + carDX)) > 0.01f ||
                    fabsf(car.position.z - (oldZ + carDZ)) > 0.01f) {
                    car.speed *= -0.20f;
                }
            }
        }

        // Collect the next delivery marker when the player or car reaches it.
        Vector3 actorPosition = inCar ? car.position : player;
        if (missionIndex < 3 &&
            Distance2D(actorPosition.x, actorPosition.z,
                       missionPoints[missionIndex].x,
                       missionPoints[missionIndex].z) < 5.5f) {
            missionIndex++;
            cash += 500;
        }

        // Third-person camera stays behind the current actor.
        actorPosition = inCar ? car.position : player;
        float actorYaw = inCar ? car.yaw : playerYaw;
        Vector3 cameraTarget = {actorPosition.x, actorPosition.y + 1.1f, actorPosition.z};
        camera.position = (Vector3){
            actorPosition.x - sinf(actorYaw) * 10.5f,
            actorPosition.y + 6.2f,
            actorPosition.z + cosf(actorYaw) * 10.5f
        };
        camera.target = cameraTarget;

        BeginDrawing();
        ClearBackground((Color){127, 190, 224, 255});
        BeginMode3D(camera);

        DrawPlane((Vector3){0.0f, -0.06f, 0.0f}, (Vector2){280.0f, 280.0f}, COLOR_GRASS);
        DrawRoads();
        DrawBuildings(buildings, buildingCount);

        // Decorative street trees at selected corners.
        for (int x = -2; x <= 2; x++) {
            for (int z = -2; z <= 2; z++) {
                if ((x + z) % 2 == 0) {
                    float tx = (float)x * 40.0f + 7.0f;
                    float tz = (float)z * 40.0f + 7.0f;
                    DrawCylinder((Vector3){tx, 1.0f, tz}, 0.20f, 0.28f, 2.0f, 8,
                                (Color){105, 73, 48, 255});
                    DrawSphere((Vector3){tx, 2.5f, tz}, 1.25f,
                               (Color){49, 119, 62, 255});
                }
            }
        }

        if (missionIndex < 3) DrawMissionMarker(missionPoints[missionIndex], missionIndex + 1);
        if (inCar) DrawVehicle(&car);
        else {
            DrawVehicle(&car);
            DrawPlayer(player, playerYaw, walkPhase);
        }

        EndMode3D();

        // Heads-up display
        DrawRectangleRounded((Rectangle){16.0f, 14.0f, 330.0f, 112.0f},
                             0.08f, 8, COLOR_HUD);
        DrawText("CITY LIFE 3D", 30, 24, 25, RAYWHITE);
        DrawText(TextFormat("CASH: $%d", cash), 30, 60, 18, COLOR_GOLD);
        if (missionIndex < 3) {
            DrawText(TextFormat("DELIVERY: %d / 3", missionIndex), 30, 88, 17, RAYWHITE);
        } else {
            DrawText("ALL DELIVERIES COMPLETE!", 30, 88, 16, GREEN);
        }

        if (inCar) {
            DrawRectangleRounded((Rectangle){(float)screenW - 190.0f, 16.0f, 174.0f, 58.0f},
                                 0.16f, 8, COLOR_HUD);
            DrawText(TextFormat("SPEED %03d", (int)fabsf(car.speed)),
                     screenW - 174, 34, 20, RAYWHITE);
        }

        // Virtual joystick background and moving knob.
        DrawCircle((int)stickX, (int)stickY, 66.0f, Fade(DARKGRAY, 0.48f));
        DrawCircleLines((int)stickX, (int)stickY, 66.0f, Fade(RAYWHITE, 0.9f));
        DrawCircle((int)(stickX + input.x * 35.0f),
                   (int)(stickY - input.y * 35.0f), 25.0f,
                   Fade(RAYWHITE, 0.78f));

        const char *actionLabel = inCar ? "EXIT" :
            (Distance2D(player.x, player.z, car.position.x, car.position.z) < 5.5f ? "GET CAR" : "JUMP");
        DrawButton(actionRect, actionLabel, (Color){38, 104, 176, 255});
        DrawButton(gasRect, "GAS", (Color){39, 143, 81, 255});
        DrawButton(brakeRect, "BRAKE", (Color){166, 67, 55, 255});

        DrawText("LEFT STICK: MOVE / STEER", 18, screenH - 26, 14, RAYWHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
