/*
 * qsopt_wrapper.c
 * 
 * Simple wrapper to expose qsopt functions for ctypes.
 * Compile with: gcc -shared -fPIC -o libqsopt_wrapper.so qsopt_wrapper.c qsopt.a -lm
 */

#include "qsopt.h"
#include <stdlib.h>

/*
 * Wrapper functions to handle Python ctypes calling conventions
 * These are simple pass-throughs that ctypes can call directly
 */

/* Problem creation and destruction */
QSprob wrap_QScreate_prob(const char *name, int objsense) {
    return QScreate_prob(name, objsense);
}

void wrap_QSfree_prob(QSprob p) {
    QSfree_prob(p);
}

/* Problem information */
int wrap_QSget_colcount(QSprob p) {
    return QSget_colcount(p);
}

int wrap_QSget_rowcount(QSprob p) {
    return QSget_rowcount(p);
}

/* Modification functions */
int wrap_QSnew_col(QSprob p, double obj, double lower, double upper, const char *name) {
    return QSnew_col(p, obj, lower, upper, name);
}

int wrap_QSnew_row(QSprob p, double rhs, char sense, const char *name) {
    return QSnew_row(p, rhs, sense, name);
}

int wrap_QSadd_cols(QSprob p, int num, int *cmatcnt, int *cmatbeg, int *cmatind,
                    double *cmatval, double *obj, double *lower, double *upper,
                    const char **names) {
    return QSadd_cols(p, num, cmatcnt, cmatbeg, cmatind, cmatval, obj, lower, upper, names);
}

int wrap_QSadd_rows(QSprob p, int num, int *rmatcnt, int *rmatbeg, int *rmatind,
                    double *rmatval, double *rhs, char *sense, const char **names) {
    return QSadd_rows(p, num, rmatcnt, rmatbeg, rmatind, rmatval, rhs, sense, names);
}

int wrap_QSadd_col(QSprob p, int cnt, int *cmatind, double *cmatval, 
                   double obj, double lower, double upper, const char *name) {
    return QSadd_col(p, cnt, cmatind, cmatval, obj, lower, upper, name);
}

int wrap_QSadd_row(QSprob p, int cnt, int *rmatind, double *rmatval, 
                   double rhs, char sense, const char *name) {
    return QSadd_row(p, cnt, rmatind, rmatval, rhs, sense, name);
}

/* Solving */
int wrap_QSopt_primal(QSprob p, int *status) {
    return QSopt_primal(p, status);
}

int wrap_QSopt_dual(QSprob p, int *status) {
    return QSopt_dual(p, status);
}

/* Solution retrieval */
int wrap_QSget_status(QSprob p, int *status) {
    return QSget_status(p, status);
}

int wrap_QSget_objval(QSprob p, double *value) {
    return QSget_objval(p, value);
}

int wrap_QSget_x_array(QSprob p, double *x) {
    return QSget_x_array(p, x);
}

int wrap_QSget_solution(QSprob p, double *value, double *x, double *pi,
                        double *slack, double *rc) {
    return QSget_solution(p, value, x, pi, slack, rc);
}

/* Parameter setting */
int wrap_QSchange_objsense(QSprob p, int newsense) {
    return QSchange_objsense(p, newsense);
}

int wrap_QSchange_bound(QSprob p, int indx, char lu, double bound) {
    return QSchange_bound(p, indx, lu, bound);
}

int wrap_QSchange_coef(QSprob p, int rowindex, int colindex, double coef) {
    return QSchange_coef(p, rowindex, colindex, coef);
}

int wrap_QSchange_objcoef(QSprob p, int indx, double coef) {
    return QSchange_objcoef(p, indx, coef);
}

int wrap_QSchange_rhscoef(QSprob p, int indx, double coef) {
    return QSchange_rhscoef(p, indx, coef);
}

int wrap_QSset_param(QSprob p, int whichparam, int newvalue) {
    return QSset_param(p, whichparam, newvalue);
}

int wrap_QSset_param_double(QSprob p, int whichparam, double newvalue) {
    return QSset_param_double(p, whichparam, newvalue);
}

/* Version info */
const char* wrap_QSversion(void) {
    return QSversion();
}
