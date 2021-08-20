/*
    ArduinoGL.h - OpenGL subset for Arduino.
    Created by Fabio de Albuquerque Dela Antonio
    fabio914 at gmail.com
 */

#include <Adafruit_GFX.h>
#include "ArduinoGL.h"
#include "MyCanvas.h"
#include "color.h"
#define MAX_VERTICES 24
#define MAX_MATRICES 8

#define DEG2RAD (3.14159265358979323846 / 180.0)

MyCanvas *glCanvas = NULL;
GLDrawMode glDrawMode = GL_NONE;

GLVertex glVertices[MAX_VERTICES];
unsigned glVerticesCount = 0;

GLMatrixMode glmatrixMode = GL_PROJECTION;
float glMatrices[2][16];
float glMatrixStack[MAX_MATRICES][16];
unsigned glMatrixStackTop = 0;

unsigned glPointLength = 1;

/* Aux functions */
void copyMatrix(float *dest, float *src) { std::copy(src, src + 16, dest); }

void multMatrix(float *dest, float *src1, float *src2) {
    float tmp[16];
    float *tgt = dest;
    if (dest == src1 || dest == src2) {
        tgt = tmp;
    }

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            tgt[i + j * 4] = 0.0;
            for (int k = 0; k < 4; k++) {
                tgt[i + j * 4] += src1[i + k * 4] * src2[k + j * 4];
            }
        }
    }

    if (tgt == tmp) {
        copyMatrix(dest, tmp);
    }
}

void pushMatrix(float *m) {
    if (glMatrixStackTop < MAX_MATRICES) {
        copyMatrix(glMatrixStack[glMatrixStackTop], m);
        glMatrixStackTop++;
    }
}

void popMatrix(void) {
    if (glMatrixStackTop > 0) {
        glMatrixStackTop--;
    }
}

GLVertex multVertex(float *m, GLVertex v) {
    GLVertex ret;

    ret.x = m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12] * v.w;
    ret.y = m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13] * v.w;
    ret.z = m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14] * v.w;
    ret.w = m[3] * v.x + m[7] * v.y + m[11] * v.z + m[15] * v.w;

    return ret;
}

void normVector3(float *dest, float *src) {
    float norm = sqrt(src[0] * src[0] + src[1] * src[1] + src[2] * src[2]);

    for (int i = 0; i < 3; i++) {
        dest[i] = src[i] / norm;
    }
}

void crossVector3(float *dest, float *src1, float *src2) {
    float tmp[3];
    float *tgt = dest;
    if (dest == src1 || dest == src2) {
        tgt = tmp;
    }

    tgt[0] = src1[1] * src2[2] - src1[2] * src2[1];
    tgt[1] = src1[2] * src2[0] - src1[0] * src2[2];
    tgt[2] = src1[0] * src2[1] - src1[1] * src2[0];

    if (tgt == tmp) {
        std::copy(tgt, tgt + 3, dest);
    }
}

/* Matrices */

void glMatrixMode(GLMatrixMode mode) {
    if (mode == GL_MODELVIEW || mode == GL_PROJECTION) {
        glmatrixMode = mode;
    }
}

void glMultMatrixf(float *m) { multMatrix(glMatrices[glmatrixMode], glMatrices[glmatrixMode], m); }

void glLoadMatrixf(float *m) { copyMatrix(glMatrices[glmatrixMode], m); }

void glLoadIdentity(void) {
    float m[16];

    for (int i = 0; i < 16; i++) {
        if (i % 5 == 0) {
            m[i] = 1.0;
        } else {
            m[i] = 0.0;
        }
    }

    glLoadMatrixf(m);
}

void glPushMatrix(void) { pushMatrix(glMatrices[glmatrixMode]); }

void glPopMatrix(void) { popMatrix(); }

void glOrtho(float left, float right, float bottom, float top, float zNear, float zFar) {
    float tx = -(right + left) / (right - left);
    float ty = -(top + bottom) / (top - bottom);
    float tz = -(zFar + zNear) / (zFar - zNear);

    float m[16];

    for (int i = 0; i < 16; i++) {
        m[i] = 0.0;
    }

    m[0] = 2.0 / (right - left);
    m[5] = 2.0 / (top - bottom);
    m[10] = -2.0 / (zFar - zNear);
    m[12] = tx;
    m[13] = ty;
    m[14] = tz;
    m[15] = 1.0;

    glMultMatrixf(m);
}

void gluOrtho2D(float left, float right, float bottom, float top) { glOrtho(left, right, bottom, top, -1.0, 1.0); }

void glFrustum(float left, float right, float bottom, float top, float zNear, float zFar) {
    float A = (right + left) / (right - left);
    float B = (top + bottom) / (top - bottom);
    float C = -(zFar + zNear) / (zFar - zNear);
    float D = -(2.0 * zFar * zNear) / (zFar - zNear);

    float m[16];

    for (int i = 0; i < 16; i++) {
        m[i] = 0.0;
    }

    m[0] = (2.0 * zNear) / (right - left);
    m[5] = (2.0 * zNear) / (top - bottom);
    m[8] = A;
    m[9] = B;
    m[10] = C;
    m[11] = -1.0;
    m[14] = D;

    glMultMatrixf(m);
}

void gluPerspective(float fovy, float aspect, float zNear, float zFar) {
    float aux = tan((fovy / 2.0) * DEG2RAD);
    float top = zNear * aux;
    float bottom = -top;
    float right = zNear * aspect * aux;
    float left = -right;

    glFrustum(left, right, bottom, top, zNear, zFar);
}

void glRotatef(float angle, float x, float y, float z) {
    float c = cos(DEG2RAD * angle), s = sin(DEG2RAD * angle);
    float nx, ny, nz, norm;

    norm = sqrt(x * x + y * y + z * z);

    if (norm == 0)
        return;

    nx = x / norm;
    ny = y / norm;
    nz = z / norm;

    float m[16];

    for (int i = 0; i < 16; i++) {
        m[i] = 0.0;
    }

    m[0] = nx * nx * (1.0 - c) + c;
    m[1] = ny * nx * (1.0 - c) + nz * s;
    m[2] = nx * nz * (1.0 - c) - ny * s;
    m[4] = nx * ny * (1.0 - c) - nz * s;
    m[5] = ny * ny * (1.0 - c) + c;
    m[6] = ny * nz * (1.0 - c) + nx * s;
    m[8] = nx * nz * (1.0 - c) + ny * s;
    m[9] = ny * nz * (1.0 - c) - nx * s;
    m[10] = nz * nz * (1.0 - c) + c;
    m[15] = 1.0;

    glMultMatrixf(m);
}

void glTranslatef(float x, float y, float z) {
    float m[16];

    for (int i = 0; i < 16; i++) {
        m[i] = 0.0;
    }

    m[0] = 1.0;
    m[5] = 1.0;
    m[10] = 1.0;
    m[12] = x;
    m[13] = y;
    m[14] = z;
    m[15] = 1.0;

    glMultMatrixf(m);
}

void glScalef(float x, float y, float z) {
    float m[16];

    for (int i = 0; i < 16; i++) {
        m[i] = 0.0;
    }

    m[0] = x;
    m[5] = y;
    m[10] = z;
    m[15] = 1.0;

    glMultMatrixf(m);
}

void gluLookAt(float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY,
               float upZ) {
    float dir[3], up[3];

    dir[0] = centerX - eyeX;
    dir[1] = centerY - eyeY;
    dir[2] = centerZ - eyeZ;

    up[0] = upX;
    up[1] = upY;
    up[2] = upZ;

    float n[3], u[3], v[3];

    normVector3(n, dir);

    crossVector3(u, n, up);
    normVector3(u, u);

    crossVector3(v, u, n);

    float m[16];

    for (int i = 0; i < 16; i++) {
        m[i] = 0.0;
    }

    m[0] = u[0];
    m[1] = v[0];
    m[2] = -n[0];

    m[4] = u[1];
    m[5] = v[1];
    m[6] = -n[1];

    m[8] = u[2];
    m[9] = v[2];
    m[10] = -n[2];

    m[15] = 1.0;

    glMultMatrixf(m);
    glTranslatef(-eyeX, -eyeY, -eyeZ);
}

/* Vertices */

void glVertex4fv(float *v) { glVertex4f(v[0], v[1], v[2], v[3]); }

void glVertex4f(float x, float y, float z, float w) {
    glVertex4f(x, y, z, w, Color565(0xFFFF));
}

void glVertex4f(float x, float y, float z, float w, Color565 color) {
    GLVertex v;

    v.x = x;
    v.y = y;
    v.z = z;
    v.w = w;
    v.color = color;

    if (glVerticesCount < MAX_VERTICES) {
        glVertices[glVerticesCount] = v;
        glVerticesCount++;
    }
}

void glVertex3fv(float *v) { glVertex3f(v[0], v[1], v[2]); }

void glVertex3f(float x, float y, float z, Color565 color) { glVertex4f(x, y, z, 1.0, color); }
void glVertex3f(float x, float y, float z) { glVertex4f(x, y, z, 1.0, Color565(0xFFFF)); }
void glVertex3f(abstract_point point) { glVertex4f(point.x, point.y, point.z, 1.0, point.color); }

/* OpenGL */

void glUseCanvas(MyCanvas *c) { glCanvas = c; }

void glClear(int mask) {
    if (mask & GL_COLOR_BUFFER_BIT && glCanvas != nullptr) {
        glCanvas->fillScreen(0);
    }
}

void glPointSize(unsigned size) { glPointLength = size; }

void glBegin(GLDrawMode mode) {
    glDrawMode = mode;
    glVerticesCount = 0;
}

void glEnd(void) {

    if (glCanvas == nullptr || glDrawMode == GL_NONE)
        return;

    float modelviewProjection[16];
    multMatrix(modelviewProjection, glMatrices[GL_PROJECTION], glMatrices[GL_MODELVIEW]);
    int frameWidth = glCanvas->width();
    int frameHeight = glCanvas->height();
    float vertex_distance[glVerticesCount];
    float vertex_brightness[glVerticesCount];
    for (unsigned int i = 0; i < glVerticesCount; i++) {
        GLVertex aux = multVertex(modelviewProjection, glVertices[i]);
        aux.x = aux.x / aux.w;
        aux.y = aux.y / aux.w;
        aux.z = aux.z / aux.w;
        aux.color = glVertices[i].color;

        glVertices[i] = aux;
        // this is shitty and inefficient but i think this is what i need here
        GLVertex pre_project = multVertex(glMatrices[GL_MODELVIEW], glVertices[i]);
        // bunch of magic numbers come from the actual 3d scene and observer positions
        float light_start_distance = 39592.f;
        float distance = (pre_project.x * pre_project.x) + (pre_project.y * pre_project.y) + (pre_project.z * pre_project.z);
        vertex_distance[i] = distance;
        float apparent_bri = 1.f / (1.f + (distance - light_start_distance) / 12000.f); // 65800
        vertex_brightness[i] = max(min(apparent_bri, 1), 0.039215686); // this bottom value is the minimum to keep a LED lit
        //Serial.print(distance);
        //Serial.print(", ");
        //Serial.println(apparent_bri);
    }

    if (glDrawMode == GL_POINTS) {

        for (unsigned int i = 0; i < glVerticesCount; i++) {
            GLVertex *aux = &(glVertices[i]);

            if (!(glVertices[i].x >= -1.0 && glVertices[i].x <= 1.0))
                continue;

            if (!(glVertices[i].y >= -1.0 && glVertices[i].y <= 1.0))
                continue;

            if (!(glVertices[i].z >= -1.0 && glVertices[i].z <= 1.0))
                continue;

            int px = (((aux->x + 1.0) / 2.0) * (frameWidth - 1));
            int py = ((1.0 - ((aux->y + 1.0) / 2.0)) * (frameHeight - 1));

            for (int x = (px - glPointLength / 2.0); x <= (px + glPointLength / 2.0); x++)
                for (int y = (py - glPointLength / 2.0); y <= (py + glPointLength / 2.0); y++)
                    glCanvas->drawPixel(x, y, 0x1);
        }
    }

    else if (glDrawMode == GL_POLYGON) {

        /* TODO Improve! */
        if (glVerticesCount < 2)
            return;

        int px[MAX_VERTICES], py[MAX_VERTICES];
        Color565 pcolor[MAX_VERTICES];

        for (unsigned int i = 0; i < glVerticesCount; i++) {
            if (!(glVertices[i].z >= -1.0 && glVertices[i].z <= 1.0))
                return;

            GLVertex *aux = &(glVertices[i]);

            px[i] = (((aux->x + 1.0) / 2.0) * (frameWidth - 1));
            py[i] = ((1.0 - ((aux->y + 1.0) / 2.0)) * (frameHeight - 1));
            pcolor[i] = aux->color;
            Serial.print(pcolor[i]);
            Serial.print(", ");
            Serial.println(vertex_brightness[i]);
        }

        for (unsigned int i = 0; i < glVerticesCount; i++) {
            int next = (i + 1u == glVerticesCount) ? 0 : (i + 1);
            // TODO: replace with more sophisticated lighting function
            // include z coordinate in function signature
            // vertices now have a assigned color so we need to read and re-scale that
            glCanvas->drawShadedLine(
                px[i], py[i], vertex_distance[i], 
                px[next], py[next], vertex_distance[next], 
                pcolor[i].dim(vertex_brightness[i]), 
                pcolor[next].dim(vertex_brightness[next]));
        }
    }

    else if (glDrawMode == GL_TRIANGLE_STRIP) {

        /* TODO Improve! */
        if (glVerticesCount < 3u)
            return;

        int px[MAX_VERTICES], py[MAX_VERTICES];

        for (unsigned int i = 0; i < glVerticesCount; i++) {
            if (!(glVertices[i].z >= -1.0 && glVertices[i].z <= 1.0))
                return;

            GLVertex *aux = &(glVertices[i]);

            px[i] = (((aux->x + 1.0) / 2.0) * (frameWidth - 1));
            py[i] = ((1.0 - ((aux->y + 1.0) / 2.0)) * (frameHeight - 1));
        }

        for (unsigned int i = 0; i < glVerticesCount - 2; i++) {
            glCanvas->drawLine(px[i], py[i], px[i + 1], py[i + 1], 0x1);
            glCanvas->drawLine(px[i], py[i], px[i + 2], py[i + 2], 0x1);
            glCanvas->drawLine(px[i + 1], py[i + 1], px[i + 2], py[i + 2], 0x1);
        }
    }
}
