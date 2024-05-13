//------------------------------------------------------------------------------
//  cube-emsc.c
//  Shader uniforms updates.
//------------------------------------------------------------------------------
#define HANDMADE_MATH_IMPLEMENTATION
#define HANDMADE_MATH_NO_SSE
#include "HandmadeMath.h"
#define SOKOL_IMPL
#define SOKOL_GLES3
#include "sokol_gfx.h"
#include "sokol_log.h"
#include "emsc.h"

static struct {
  float x, z;
  sg_pipeline pip;
  sg_bindings bind;
  sg_pass_action pass_action;
} state = {.pass_action.colors[0] = {.load_action = SG_LOADACTION_CLEAR,
                                     .clear_value = {0.0f, 0.0f, 0.0f, 1.0f}}};

typedef struct {
  hmm_mat4 mvp;
} params_t;

static EM_BOOL draw(double time, void* userdata);

int main() {
  // setup WebGL context
  emsc_init("#canvas", EMSC_ANTIALIAS);

  // setup sokol_gfx
  sg_setup(
      &(sg_desc){.environment = emsc_environment(), .logger.func = slog_func});
  assert(sg_isvalid());

  // cube vertex buffer
  float vertices[] = {
      -1.0, -1.0, -1.0, 1.0,  0.0,  0.0,  1.0,  1.0,  -1.0, -1.0,
      1.0,  0.0,  0.0,  1.0,  1.0,  1.0,  -1.0, 1.0,  0.0,  0.0,
      1.0,  -1.0, 1.0,  -1.0, 1.0,  0.0,  0.0,  1.0,

      -1.0, -1.0, 1.0,  0.0,  1.0,  0.0,  1.0,  1.0,  -1.0, 1.0,
      0.0,  1.0,  0.0,  1.0,  1.0,  1.0,  1.0,  0.0,  1.0,  0.0,
      1.0,  -1.0, 1.0,  1.0,  0.0,  1.0,  0.0,  1.0,

      -1.0, -1.0, -1.0, 0.0,  0.0,  1.0,  1.0,  -1.0, 1.0,  -1.0,
      0.0,  0.0,  1.0,  1.0,  -1.0, 1.0,  1.0,  0.0,  0.0,  1.0,
      1.0,  -1.0, -1.0, 1.0,  0.0,  0.0,  1.0,  1.0,

      1.0,  -1.0, -1.0, 1.0,  0.5,  0.0,  1.0,  1.0,  1.0,  -1.0,
      1.0,  0.5,  0.0,  1.0,  1.0,  1.0,  1.0,  1.0,  0.5,  0.0,
      1.0,  1.0,  -1.0, 1.0,  1.0,  0.5,  0.0,  1.0,

      -1.0, -1.0, -1.0, 0.0,  0.5,  1.0,  1.0,  -1.0, -1.0, 1.0,
      0.0,  0.5,  1.0,  1.0,  1.0,  -1.0, 1.0,  0.0,  0.5,  1.0,
      1.0,  1.0,  -1.0, -1.0, 0.0,  0.5,  1.0,  1.0,

      -1.0, 1.0,  -1.0, 1.0,  0.0,  0.5,  1.0,  -1.0, 1.0,  1.0,
      1.0,  0.0,  0.5,  1.0,  1.0,  1.0,  1.0,  1.0,  0.0,  0.5,
      1.0,  1.0,  1.0,  -1.0, 1.0,  0.0,  0.5,  1.0};
  state.bind.vertex_buffers[0] =
      sg_make_buffer(&(sg_buffer_desc){.data = SG_RANGE(vertices)});

  // create an index buffer for the cube
  uint16_t indices[] = {0,  1,  2,  0,  2,  3,  6,  5,  4,  7,  6,  4,
                        8,  9,  10, 8,  10, 11, 14, 13, 12, 15, 14, 12,
                        16, 17, 18, 16, 18, 19, 22, 21, 20, 23, 22, 20};
  state.bind.index_buffer = sg_make_buffer(&(sg_buffer_desc){
      .type = SG_BUFFERTYPE_INDEXBUFFER, .data = SG_RANGE(indices)});

  // create shader
  sg_shader shd = sg_make_shader(&(sg_shader_desc){
      .attrs = {[0].name = "position", [1].name = "color0"},
      .vs.uniform_blocks[0] =
          {.size = sizeof(params_t),
           .uniforms = {[0] = {.name = "mvp", .type = SG_UNIFORMTYPE_MAT4}}},
      .vs.source = "uniform mat4 mvp;\n"
                   "attribute vec4 position;\n"
                   "attribute vec4 color0;\n"
                   "varying vec4 color;\n"
                   "void main() {\n"
                   "  gl_Position = mvp * position;\n"
                   "  color = color0;\n"
                   "}\n",
      .fs.source = "precision mediump float;\n"
                   "varying vec4 color;\n"
                   "void main() {\n"
                   "  gl_FragColor = color;\n"
                   "}\n"});

  // create pipeline object
  state.pip = sg_make_pipeline(&(sg_pipeline_desc){
      .layout =
          {// test to provide buffer stride, but no attr offsets
           .buffers[0].stride = 28,
           .attrs = {[0].format = SG_VERTEXFORMAT_FLOAT3,
                     [1].format = SG_VERTEXFORMAT_FLOAT4}},
      .shader = shd,
      .index_type = SG_INDEXTYPE_UINT16,
      .depth = {.compare = SG_COMPAREFUNC_LESS_EQUAL, .write_enabled = true},
      .cull_mode = SG_CULLMODE_BACK});

  state.x = 4.0;
  state.z = 4.0;

  // hand off control to browser loop
  emscripten_request_animation_frame_loop(draw, 0);
  return 0;
}

// draw one frame
static EM_BOOL draw(double time, void* userdata) {
  (void)time;
  (void)userdata;
  // compute model-view-projection matrix for vertex shader
  hmm_mat4 proj = HMM_Perspective(
      60.0f, (float)emsc_width() / (float)emsc_height(), 0.01f, 10.0f);
  hmm_mat4 view =
      HMM_LookAt(HMM_Vec3(HMM_SINF(state.x) * 4, 1.5f, HMM_COSF(state.z) * 4),
                 HMM_Vec3(0.0f, 0.0f, 0.0f), HMM_Vec3(0.0f, 1.0f, 0.0f));

  hmm_mat4 view_proj = HMM_MultiplyMat4(proj, view);

  state.x += 0.016f;
  state.z += 0.016f;
  const params_t vs_params = {.mvp = view_proj};

  // ...and draw
  sg_begin_pass(
      &(sg_pass){.action = state.pass_action, .swapchain = emsc_swapchain()});
  sg_apply_pipeline(state.pip);
  sg_apply_bindings(&state.bind);
  sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &SG_RANGE(vs_params));
  sg_draw(0, 36, 1);
  sg_end_pass();
  sg_commit();
  return EM_TRUE;
}