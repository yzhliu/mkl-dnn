/*******************************************************************************
* Copyright 2016-2017 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#ifndef JIT_PRIMITIVE_CONF_HPP
#define JIT_PRIMITIVE_CONF_HPP

#include <stdint.h>

namespace mkldnn {
namespace impl {
namespace cpu {

/* convolution */
enum conv_version_t {ver_unused, ver_fma, ver_avx512_core, ver_4fma, ver_4vnni};
enum conv_loop_order_t {loop_cgn, loop_gnc, loop_ngc};
enum conv_1x1_loop_order_t {loop_rbl, loop_rlb, loop_lbr, loop_lrb, loop_blr,
                            loop_brl};
enum conv_kernel_kind_t {embd_bcast, expl_bcast};

enum {
    FLAG_MB_FIRST = 1 << 0, FLAG_MB_LAST = 1 << 1,
    FLAG_OC_FIRST = 1 << 2, FLAG_OC_LAST = 1 << 3,
    FLAG_IC_FIRST = 1 << 4, FLAG_IC_LAST = 1 << 5,
    FLAG_SP_FIRST = 1 << 6, FLAG_SP_LAST = 1 << 7,
    FLAG_REDUCE_FIRST = 1<<8, FLAG_REDUCE_LAST = 1<<9,
};

struct jit_conv_conf_t {
    prop_kind_t prop_kind;
    conv_version_t ver;
    conv_loop_order_t loop_order;

    int mb;
    int ngroups, ic, oc;
    int ih, iw, oh, ow;
    int l_pad, t_pad;
    int r_pad, b_pad;
    int kh, kw;
    int stride_h, stride_w;
    int dilate_h, dilate_w;
    memory_format_t src_fmt;
    bool with_bias, with_relu;
    float relu_negative_slope;
    bool with_sum;

    int ihp, iwp, ohp, owp;
    int nb_ic, ic_block;
    int nb_oc, oc_block;
    int nb_ic_blocking, nb_oc_blocking; // blocking of nb_ic and nb_ic
    int nb_ic_blocking_max;
    int nb_ic_L2;
    int nb_oc_L2;
    int ur_h, ur_w;
    int ur_w_tail;
    bool is_1stconv;
    /* fma avx512_core */
    conv_kernel_kind_t kernel_kind;
    /* 4fma */
    int tr_iw;
    int tr_src_num_guard_elems;
    /* 1st conv: 4fma */
    int tr_ld;
    int kh_step;
    /* 4vnni */
    int typesize_in;
    int typesize_out;
    /* avx512_u8s8u8 */
    int ic_nb1, ic_nb2;
    int oc_nb1;
    int ur_ow_max, ur_ow, ur_ow_tail;
    int ur_ow_nsteps;
    data_type_t bia_dt;
    data_type_t dst_dt;
    /* avx512: max possible value is nregs(32) - aux_regs(4) */
    int src_offsets[28];
    int src_count;
    bool expl_bcast;
    bool large_spatial;

    void print_str() {
        fprintf(stderr, "--- jit_conv_conf_t --- \n"
        "prop_kind=%d\n"
        "ver=%d\n"
        "loop_order=%d\n"
        "mb = %d\n"
        "ngroups=%d, ic=%d, oc=%d\n"
        "ih=%d, iw=%d, oh=%d, ow=%d\n"
        "l_pad=%d, t_pad=%d\n"
        "r_pad=%d, b_pad=%d\n"
        "kh=%d, kw=%d\n"
        "stride_h=%d, stride_w=%d\n"
        "dilate_h=%d, dilate_w=%d\n"
        "src_fmt=%d\n"
        "with_bias=%d, with_relu=%d\n"
        "relu_negative_slope=%.2f\n"
        "with_sum=%d\n"
        "ihp=%d, iwp=%d, ohp=%d, owp=%d\n"
        "nb_ic=%d, ic_block=%d\n"
        "nb_oc=%d, oc_block=%d\n"
        "nb_ic_blocking=%d, nb_oc_blocking=%d\n"
        "nb_ic_blocking_max=%d\n"
        "nb_ic_L2=%d, nb_oc_L2=%d\n"
        "ur_h=%d, ur_w=%d\n"
        "ur_w_tail=%d\n"
        "is_1stconv=%d\n"
        "kernel_kind=%d\n"
        "tr_iw=%d\n"
        "tr_src_num_guard_elems=%d\n"
        "tr_ld=%d\n"
        "kh_step=%d\n"
        "typesize_in=%d, typesize_out=%d\n"
        "ic_nb1=%d, ic_nb2=%d\n"
        "oc_nb1=%d\n"
        "ur_ow_max=%d, ur_ow=%d, ur_ow_tail=%d\n"
        "ur_ow_nsteps=%d\n"
        "bia_dt=%d, dst_dt=%d\n"
        "src_count=%d\n"
        "expl_bcast=%d, large_spatial=%d\n", prop_kind, ver, loop_order,
        mb, ngroups, ic, oc, ih, iw, oh, ow,
        l_pad, t_pad, r_pad, b_pad, kh, kw, stride_h, stride_w, dilate_h, dilate_w,
        src_fmt, with_bias, with_relu, relu_negative_slope, with_sum, ihp, iwp, ohp, owp,
        nb_ic, ic_block, nb_oc, oc_block, nb_ic_blocking, nb_oc_blocking,
        nb_ic_blocking_max, nb_ic_L2, nb_oc_L2, ur_h, ur_w, ur_w_tail, is_1stconv,
        kernel_kind, tr_iw, tr_src_num_guard_elems, tr_ld, kh_step, typesize_in, typesize_out,
        ic_nb1, ic_nb2, oc_nb1, ur_ow_max, ur_ow, ur_ow_tail, ur_ow_nsteps, bia_dt, dst_dt,
        src_count, expl_bcast, large_spatial);
    }
};

/*
   Winograd sched policy:

   Computation Unit:
   W: weights transform
   S: src transform
   D: dst transform
   G: gemm

   Thread grouping by:
   i: nb_ic
   o: nb_oc
   t: tile_block
   e: element in tile

   Note: 'i' and 'o' are omited if
   i. not comblined with t or
   ii. with discrete transforms

   Current policies supported:
*/
enum winograd_sched_t {
    WSCHED_INVALID = 0,

    /* Forward & backward-data */
    /* W_S_G_D implements discrete transforms */
    WSCHED_DATA_W_S_G_D,
    /* W_SGD implements tiled transforms s.t. GEMM could reuse data in L2*/
    WSCHED_DATA_W_SGD,

    /* Backward-weights */
    WSCHED_WEI_S_D_G_W,
    WSCHED_WEI_S_D_Giot_W,
    WSCHED_WEI_SDGtWo,
    WSCHED_WEI_SDGt_W,
};

struct jit_conv_winograd_conf_t : public jit_conv_conf_t {
    //alpha determines the tile size
    static const int alpha = 6;
    int itiles;
    int jtiles;
    int ntiles;
    int ic_simd_block;
    int tile_4fma_padding;
    int tile_4fma;
    int oc_simd_block;
    int tile_block;
    int tile_block_ur;
    int nb_tile_block_ur;

    bool double_buffering;
    int zmm_start;
    int nb_reg;

    int dimK;
    int dimK_4fma;
    int dimK_reg_block;
    int dimK_block;
    int dimK_nb_block;

    int dimM;
    int dimM_simd_block;
    int dimM_block;
    int dimM_nb_block;

    int dimN;
    int dimN_reg_block;
    int dimN_block;
    int dimN_nb_block;

    winograd_sched_t sched_policy;
};

struct jit_conv_call_s {
    const void *src; /* hack, non-const for backward_data */
    const void *dst; /* hack, non-const for forward */
    const void *filt; /* hack, non-const for backward_weights */
    const void *bias; /* hack, non-const for backward_bias */
    const void *src_prf;
    const void *dst_prf;
    const void *filt_prf;
    const void *bias_prf;
    size_t kh_padding;
    size_t kh_padding_prf;
    size_t kw_padding;
    size_t channel;
    size_t channel_prf;
    size_t oc_blocks;
    int flags;
};

struct jit_1x1_conv_conf_t {
    prop_kind_t prop_kind;
    conv_version_t ver;

    int mb;
    int ngroups, ic, oc;
    int iw, ih, ow, oh;
    int l_pad, t_pad;
    int kh, kw;
    int stride_h, stride_w;
    memory_format_t src_fmt;
    bool with_bias, with_relu;
    float relu_negative_slope;
    bool with_sum;

    int is, os;
    int ic_block, oc_block;

    int ur, ur_tail;

    int reduce_dim, reduce_block, nb_reduce,
        nb_reduce_blocking, nb_reduce_blocking_max;
    int load_dim, load_block, nb_load,
        nb_load_blocking, nb_load_blocking_max;
    int bcast_dim, bcast_block, nb_bcast,
        nb_bcast_blocking, nb_bcast_blocking_max;

    int reduce_loop_unroll, reduce_loop_bcast_step, reduce_loop_load_step;
    int load_loop_load_step, load_loop_iter_step;
    int bcast_loop_output_step, bcast_loop_output_substep;
    int bcast_loop_bcast_step, bcast_loop_bcast_substep;
    int fma_step;
    int load_grp_count;
    conv_1x1_loop_order_t loop_order;
    bool use_vmovntps;
    /* avx512 core */
    bool expl_bcast;
    /* 4vnni */
    int typesize_in;
    int typesize_out;

    /* 4fma */
    bool transpose_src;
    int tr_is;
    int nthr, nthr_mb, nthr_g, nthr_oc_b, nthr_ic_b;
};

struct jit_gemm_conv_conf_t {
    prop_kind_t prop_kind;

    int mb;
    int ngroups, ic, oc;
    int iw, ih, ow, oh;
    int l_pad, t_pad;
    int kh, kw;
    int stride_h, stride_w;
    int dilate_h, dilate_w;
    memory_format_t src_fmt;
    bool with_bias, with_relu;
    float relu_negative_slope;

    int is, os, ks;
    int ic_block, oc_block;
    bool need_im2col;
};

struct jit_1x1_conv_call_s {
    const void *bcast_data;
    const void *load_data;
    const void *output_data;
    const void *bias_data; // used in forward and backward_weights only

    size_t load_dim;
    size_t bcast_dim;
    size_t reduce_dim;

    size_t output_stride; // used in backward_weights only

    size_t reduce_pos_flag;
};

/* pooling */
struct jit_pool_conf_t {
    int mb, c;
    int ih, iw, oh, ow;
    int stride_h, stride_w;
    int kh, kw;
    int t_pad, l_pad;
    alg_kind_t alg;
    bool is_training;
    bool pad_w_is_null;
    bool is_backward;
    data_type_t ind_dt;

    int c_block, c_tail, nb_c;
    int ur_c, ur_c_tail;
    int ur_w;
    int ur_w_tail;
    size_t tail[4];
    data_type_t src_dt;
    data_type_t dst_dt;
};

struct jit_pool_call_s {
    const float *src;
    const float *dst;
    const void *indices;
    const float *src_prf;
    const float *dst_prf;
    const void *indices_prf;
    size_t oh;
    size_t kh_padding;
    size_t kh_padding_shift;
    size_t kw_padding;
    const float* init_value;
    float ker_area_h;
};


}
}
}

#endif
