#include "math.hpp"

// clang-format off
#define MATRIX_MULT(x, y) .m##x##y = a.m##x##1 * b.m1##y + a.m##x##2 * b.m2##y + a.m##x##3 * b.m3##y + a.m##x##4 * b.m4##y
Matrix4 operator*(Matrix4& a, Matrix4& b) {
	return Matrix4{
		MATRIX_MULT(1, 1), MATRIX_MULT(2, 1), MATRIX_MULT(3, 1), MATRIX_MULT(4, 1),
		MATRIX_MULT(1, 2), MATRIX_MULT(2, 2), MATRIX_MULT(3, 2), MATRIX_MULT(4, 2),
		MATRIX_MULT(1, 3), MATRIX_MULT(2, 3), MATRIX_MULT(3, 3), MATRIX_MULT(4, 3), 
		MATRIX_MULT(1, 4), MATRIX_MULT(2, 4), MATRIX_MULT(3, 4), MATRIX_MULT(4, 4), 
	};
}
#undef MATRIX_MULT
// clang-format on
