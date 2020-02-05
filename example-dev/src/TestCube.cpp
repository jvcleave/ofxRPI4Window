#include "TestCube.h"

GLuint program = 0;
GLint modelviewmatrix, modelviewprojectionmatrix, normalmatrix;
GLuint vbo;
GLuint positionsoffset, colorsoffset, normalsoffset;

static const GLfloat vVertices[] = {
    // front
    -1.0f, -1.0f, +1.0f,
    +1.0f, -1.0f, +1.0f,
    -1.0f, +1.0f, +1.0f,
    +1.0f, +1.0f, +1.0f,
    // back
    +1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
    +1.0f, +1.0f, -1.0f,
    -1.0f, +1.0f, -1.0f,
    // right
    +1.0f, -1.0f, +1.0f,
    +1.0f, -1.0f, -1.0f,
    +1.0f, +1.0f, +1.0f,
    +1.0f, +1.0f, -1.0f,
    // left
    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, +1.0f,
    -1.0f, +1.0f, -1.0f,
    -1.0f, +1.0f, +1.0f,
    // top
    -1.0f, +1.0f, +1.0f,
    +1.0f, +1.0f, +1.0f,
    -1.0f, +1.0f, -1.0f,
    +1.0f, +1.0f, -1.0f,
    // bottom
    -1.0f, -1.0f, -1.0f,
    +1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, +1.0f,
    +1.0f, -1.0f, +1.0f,
};

static const GLfloat vColors[] = {
    // front
    0.0f,  0.0f,  1.0f, // blue
    1.0f,  0.0f,  1.0f, // magenta
    0.0f,  1.0f,  1.0f, // cyan
    1.0f,  1.0f,  1.0f, // white
    // back
    1.0f,  0.0f,  0.0f, // red
    0.0f,  0.0f,  0.0f, // black
    1.0f,  1.0f,  0.0f, // yellow
    0.0f,  1.0f,  0.0f, // green
    // right
    1.0f,  0.0f,  1.0f, // magenta
    1.0f,  0.0f,  0.0f, // red
    1.0f,  1.0f,  1.0f, // white
    1.0f,  1.0f,  0.0f, // yellow
    // left
    0.0f,  0.0f,  0.0f, // black
    0.0f,  0.0f,  1.0f, // blue
    0.0f,  1.0f,  0.0f, // green
    0.0f,  1.0f,  1.0f, // cyan
    // top
    0.0f,  1.0f,  1.0f, // cyan
    1.0f,  1.0f,  1.0f, // white
    0.0f,  1.0f,  0.0f, // green
    1.0f,  1.0f,  0.0f, // yellow
    // bottom
    0.0f,  0.0f,  0.0f, // black
    1.0f,  0.0f,  0.0f, // red
    0.0f,  0.0f,  1.0f, // blue
    1.0f,  0.0f,  1.0f  // magenta
};

static const GLfloat vNormals[] = {
    // front
    +0.0f, +0.0f, +1.0f, // forward
    +0.0f, +0.0f, +1.0f, // forward
    +0.0f, +0.0f, +1.0f, // forward
    +0.0f, +0.0f, +1.0f, // forward
    // back
    +0.0f, +0.0f, -1.0f, // backward
    +0.0f, +0.0f, -1.0f, // backward
    +0.0f, +0.0f, -1.0f, // backward
    +0.0f, +0.0f, -1.0f, // backward
    // right
    +1.0f, +0.0f, +0.0f, // right
    +1.0f, +0.0f, +0.0f, // right
    +1.0f, +0.0f, +0.0f, // right
    +1.0f, +0.0f, +0.0f, // right
    // left
    -1.0f, +0.0f, +0.0f, // left
    -1.0f, +0.0f, +0.0f, // left
    -1.0f, +0.0f, +0.0f, // left
    -1.0f, +0.0f, +0.0f, // left
    // top
    +0.0f, +1.0f, +0.0f, // up
    +0.0f, +1.0f, +0.0f, // up
    +0.0f, +1.0f, +0.0f, // up
    +0.0f, +1.0f, +0.0f, // up
    // bottom
    +0.0f, -1.0f, +0.0f, // down
    +0.0f, -1.0f, +0.0f, // down
    +0.0f, -1.0f, +0.0f, // down
    +0.0f, -1.0f, +0.0f  // down
};

static const char *vertex_shader_source = STRINGIFY(
uniform mat4 modelviewMatrix;
uniform mat4 modelviewprojectionMatrix;
uniform mat3 normalMatrix;
                                                   
attribute vec4 in_position;
attribute vec3 in_normal;
attribute vec4 in_color;
                                                   
vec4 lightSource = vec4(2.0, 2.0, 20.0, 0.0);
                                                   
varying vec4 vVaryingColor;
                                                   
void main()
{
    gl_Position = modelviewprojectionMatrix * in_position;
    vec3 vEyeNormal = normalMatrix * in_normal;
    vec4 vPosition4 = modelviewMatrix * in_position;
    vec3 vPosition3 = vPosition4.xyz / vPosition4.w;
    vec3 vLightDir = normalize(lightSource.xyz - vPosition3);
    float diff = max(0.0, dot(vEyeNormal, vLightDir));
    vVaryingColor = vec4(diff * in_color.rgb, 1.0);
}
);

static const char *fragment_shader_source = STRINGIFY(
precision mediump float;
                                             
varying vec4 vVaryingColor;
                                             
void main()
{
    gl_FragColor = vVaryingColor;
});

void TestCube::draw()
{
    int i = ofGetFrameNum();
    
    auto aspect = ofGetWidth() / ofGetHeight();
    if(program == 0)
    {
        GLint ret;
        
        GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        
        glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
        glCompileShader(vertex_shader);
        glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &ret);
        
        ofLog() << "vertex_shader: " << ret;
        
        glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
        glCompileShader(fragment_shader);
        
        glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &ret);
        
        ofLog() << "fragment_shader: " << ret;
        
        
        program = glCreateProgram();
        
        glAttachShader(program, vertex_shader);
        glAttachShader(program, fragment_shader);
        
        
        glBindAttribLocation(program, 0, "in_position");
        glBindAttribLocation(program, 1, "in_normal");
        glBindAttribLocation(program, 2, "in_color");
        
        
        glLinkProgram(program);
        
        glGetProgramiv(program, GL_LINK_STATUS, &ret);
        
        
        glUseProgram(program);
        
        modelviewmatrix = glGetUniformLocation(program, "modelviewMatrix");
        modelviewprojectionmatrix = glGetUniformLocation(program, "modelviewprojectionMatrix");
        normalmatrix = glGetUniformLocation(program, "normalMatrix");
        
        glViewport(0, 0, ofGetWidth(), ofGetHeight());
        glEnable(GL_CULL_FACE);
        positionsoffset = 0;
        colorsoffset = sizeof(vVertices);
        normalsoffset = sizeof(vVertices) + sizeof(vColors);
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vVertices) + sizeof(vColors) + sizeof(vNormals), 0, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, positionsoffset, sizeof(vVertices), &vVertices[0]);
        glBufferSubData(GL_ARRAY_BUFFER, colorsoffset, sizeof(vColors), &vColors[0]);
        glBufferSubData(GL_ARRAY_BUFFER, normalsoffset, sizeof(vNormals), &vNormals[0]);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid *)(intptr_t)positionsoffset);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid *)(intptr_t)normalsoffset);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid *)(intptr_t)colorsoffset);
        glEnableVertexAttribArray(2);
        
    }
    
    ESMatrix modelview;
    
    /* clear the color buffer */
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    
    esMatrixLoadIdentity(&modelview);
    esTranslate(&modelview, 0.0f, 0.0f, -8.0f);
    esRotate(&modelview, 45.0f + (0.25f * i), 1.0f, 0.0f, 0.0f);
    esRotate(&modelview, 45.0f - (0.5f * i), 0.0f, 1.0f, 0.0f);
    esRotate(&modelview, 10.0f + (0.15f * i), 0.0f, 0.0f, 1.0f);
    
    
    ESMatrix projection;
    esMatrixLoadIdentity(&projection);
    esFrustum(&projection, -2.8f, +2.8f, -2.8f * aspect, +2.8f * aspect, 6.0f, 10.0f);
    
    
    ESMatrix modelviewprojection;
    esMatrixLoadIdentity(&modelviewprojection);
    esMatrixMultiply(&modelviewprojection, &modelview, &projection);
    
    
    float normal[9];
    normal[0] = modelview.m[0][0];
    normal[1] = modelview.m[0][1];
    normal[2] = modelview.m[0][2];
    normal[3] = modelview.m[1][0];
    normal[4] = modelview.m[1][1];
    normal[5] = modelview.m[1][2];
    normal[6] = modelview.m[2][0];
    normal[7] = modelview.m[2][1];
    normal[8] = modelview.m[2][2];
    
    glUniformMatrix4fv(modelviewmatrix, 1, GL_FALSE, &modelview.m[0][0]);
    glUniformMatrix4fv(modelviewprojectionmatrix, 1, GL_FALSE, &modelviewprojection.m[0][0]);
    glUniformMatrix3fv(normalmatrix, 1, GL_FALSE, normal);
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 12, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 16, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 20, 4);
}
