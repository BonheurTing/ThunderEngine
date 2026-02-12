#pragma optimize("", off)

#include "Module/ModuleManager.h"
#include "CoreModule.h"
#include "PackageModule.h"
#include "FileSystem/File.h"
#include "FileSystem/FileModule.h"
#include "FileSystem/FileSystem.h"
#include "FileSystem/MemoryArchive.h"
#include "Matrix.h"
#include "Assertion.h"

namespace Thunder
{
	class CoreModule;

	void TestFileSystem()
	{
		ModuleManager::GetInstance()->LoadModule<CoreModule>();
		ModuleManager::GetInstance()->LoadModule<FileModule>();
		ModuleManager::GetInstance()->LoadModule<PackageModule>();

		IFileSystem* FileSys = FileModule::GetFileSystem("Content");
		{
			TRefCountPtr<NativeFile> file = static_cast<NativeFile*>(FileSys->Open("myfile.bin"));
			MemoryWriter archive;
			archive << 5.0f;
			archive << 4u;
			archive << "Hello World!";
			file->Write(archive.Data(), archive.Size());
		}

		{
			TRefCountPtr<NativeFile> file = static_cast<NativeFile*>(FileSys->Open("myfile.bin"));
			BinaryDataRef data = file->ReadData();
			MemoryReader archive(data.Get());
			float a;
			int b;
			String c;
			archive >> a;
			archive >> b;
			archive >> c;
			std::cout << a << ";" << b << ";" << c << std::endl;
		}

	}

	void TestImportResource()
	{
		
		PackageModule::GetModule()->ImportAll(true);
		/*const String fileName = FileModule::GetProjectRoot() + "\\Resource\\Mesh\\Cube.fbx";
		const String destPath = FileModule::GetProjectRoot() + "\\Content\\Mesh\\Cube.tasset";*/
		/*const String fileName = FileModule::GetProjectRoot() + "\\Resource\\TestPNG.png";
		const String destPath = FileModule::GetProjectRoot() + "\\Content\\TestPNG.tasset";
		ResourceModule::Import(fileName, destPath);*/
	}

	static bool NearlyEqual(float a, float b, float tolerance = 1e-5f)
	{
		return std::fabs(a - b) < tolerance;
	}

	static bool MatrixNearlyEqual(const TMatrix4f& A, const TMatrix4f& B, float tolerance = 1e-5f)
	{
		for (int32 i = 0; i < 4; i++)
			for (int32 j = 0; j < 4; j++)
				if (!NearlyEqual(A.M[i][j], B.M[i][j], tolerance))
					return false;
		return true;
	}

	void TestMatrix()
	{
		LOG("===== TestMatrix Begin =====");

		// 1. Default constructor -> identity
		{
			TMatrix4f mat;
			TAssert(mat.M[0][0] == 1.0f && mat.M[1][1] == 1.0f && mat.M[2][2] == 1.0f && mat.M[3][3] == 1.0f);
			TAssert(mat.M[0][1] == 0.0f && mat.M[0][2] == 0.0f && mat.M[0][3] == 0.0f);
			TAssert(mat.M[1][0] == 0.0f && mat.M[2][0] == 0.0f && mat.M[3][0] == 0.0f);
			LOG("[PASS] Default constructor -> identity");
		}

		// 2. Scalar constructor
		{
			TMatrix4f mat(0.0f);
			for (int32 i = 0; i < 16; i++)
				TAssert(mat.Raw[i] == 0.0f);
			LOG("[PASS] Scalar constructor");
		}

		// 3. 16-value constructor
		{
			TMatrix4f mat(
				1, 2, 3, 4,
				5, 6, 7, 8,
				9, 10, 11, 12,
				13, 14, 15, 16);
			TAssert(mat.M[0][0] == 1.0f && mat.M[0][3] == 4.0f);
			TAssert(mat.M[2][1] == 10.0f && mat.M[3][3] == 16.0f);
			LOG("[PASS] 16-value constructor");
		}

		// 4. TVector4 row constructor
		{
			TVector4f r0(1, 0, 0, 0);
			TVector4f r1(0, 1, 0, 0);
			TVector4f r2(0, 0, 1, 0);
			TVector4f r3(0, 0, 0, 1);
			TMatrix4f mat(r0, r1, r2, r3);
			TAssert(MatrixNearlyEqual(mat, TMatrix4f::Identity()));
			LOG("[PASS] TVector4 row constructor");
		}

		// 5. TVector row constructor (affine)
		{
			TVector3f x(1, 0, 0);
			TVector3f y(0, 1, 0);
			TVector3f z(0, 0, 1);
			TVector3f w(5, 6, 7);
			TMatrix4f mat(x, y, z, w);
			TAssert(mat.M[0][3] == 0.0f && mat.M[1][3] == 0.0f && mat.M[2][3] == 0.0f);
			TAssert(mat.M[3][3] == 1.0f);
			TAssert(mat.M[3][0] == 5.0f && mat.M[3][1] == 6.0f && mat.M[3][2] == 7.0f);
			LOG("[PASS] TVector row constructor (affine)");
		}

		// 6. SetIdentity
		{
			TMatrix4f mat(0.0f);
			mat.SetIdentity();
			TAssert(MatrixNearlyEqual(mat, TMatrix4f::Identity()));
			LOG("[PASS] SetIdentity");
		}

		// 7. Matrix multiplication (identity * A = A)
		{
			TMatrix4f A(
				1, 2, 3, 4,
				5, 6, 7, 8,
				9, 10, 11, 12,
				13, 14, 15, 16);
			TMatrix4f I;
			TMatrix4f Result = I * A;
			TAssert(MatrixNearlyEqual(Result, A));
			LOG("[PASS] Matrix multiply identity * A == A");
		}

		// 8. Matrix multiplication (non-trivial)
		{
			TMatrix4f A(
				1, 2, 0, 0,
				0, 1, 0, 0,
				0, 0, 1, 0,
				0, 0, 0, 1);
			TMatrix4f B(
				1, 0, 0, 0,
				3, 1, 0, 0,
				0, 0, 1, 0,
				0, 0, 0, 1);
			TMatrix4f C = A * B;
			// Row0: [1*1+2*3, 1*0+2*1, 0, 0] = [7, 2, 0, 0]
			// Row1: [0*1+1*3, 0*0+1*1, 0, 0] = [3, 1, 0, 0]
			TAssert(NearlyEqual(C.M[0][0], 7.0f));
			TAssert(NearlyEqual(C.M[0][1], 2.0f));
			TAssert(NearlyEqual(C.M[1][0], 3.0f));
			TAssert(NearlyEqual(C.M[1][1], 1.0f));
			LOG("[PASS] Matrix multiply non-trivial");
		}

		// 9. Matrix addition
		{
			TMatrix4f A(
				1, 2, 3, 4,
				5, 6, 7, 8,
				9, 10, 11, 12,
				13, 14, 15, 16);
			TMatrix4f B(
				16, 15, 14, 13,
				12, 11, 10, 9,
				8, 7, 6, 5,
				4, 3, 2, 1);
			TMatrix4f C = A + B;
			for (int32 i = 0; i < 16; i++)
				TAssert(NearlyEqual(C.Raw[i], 17.0f));
			LOG("[PASS] Matrix addition");
		}

		// 10. Scalar multiplication
		{
			TMatrix4f A(
				1, 2, 3, 4,
				5, 6, 7, 8,
				9, 10, 11, 12,
				13, 14, 15, 16);
			TMatrix4f B = A * 2.0f;
			TAssert(NearlyEqual(B.M[0][0], 2.0f));
			TAssert(NearlyEqual(B.M[3][3], 32.0f));
			LOG("[PASS] Scalar multiplication");
		}

		// 11. Equality / Inequality
		{
			TMatrix4f A;
			TMatrix4f B;
			TAssert(A == B);
			B.M[1][2] = 99.0f;
			TAssert(A != B);
			LOG("[PASS] Equality / Inequality");
		}

		// 12. Transpose
		{
			TMatrix4f A(
				1, 2, 3, 4,
				5, 6, 7, 8,
				9, 10, 11, 12,
				13, 14, 15, 16);
			TMatrix4f T = A.GetTransposed();
			for (int32 i = 0; i < 4; i++)
				for (int32 j = 0; j < 4; j++)
					TAssert(NearlyEqual(T.M[i][j], A.M[j][i]));
			// Double transpose should return original
			TMatrix4f TT = T.GetTransposed();
			TAssert(MatrixNearlyEqual(TT, A));
			LOG("[PASS] Transpose");
		}

		// 13. Determinant of identity == 1
		{
			TMatrix4f I;
			TAssert(NearlyEqual(I.Determinant(), 1.0f));
			LOG("[PASS] Determinant of identity == 1");
		}

		// 14. Determinant non-trivial
		{
			TMatrix4f A(
				1, 0, 0, 0,
				0, 2, 0, 0,
				0, 0, 3, 0,
				0, 0, 0, 4);
			TAssert(NearlyEqual(A.Determinant(), 24.0f)); // 1*2*3*4
			LOG("[PASS] Determinant non-trivial (diagonal)");
		}

		// 15. RotDeterminant
		{
			TMatrix4f I;
			TAssert(NearlyEqual(I.RotDeterminant(), 1.0f));
			LOG("[PASS] RotDeterminant");
		}

		// 16. Inverse of identity == identity
		{
			TMatrix4f I;
			TMatrix4f InvI = I.Inverse();
			TAssert(MatrixNearlyEqual(InvI, I));
			LOG("[PASS] Inverse of identity == identity");
		}

		// 17. Inverse non-trivial: A * A^-1 == I
		{
			TMatrix4f A(
				1, 2, 3, 0,
				0, 1, 4, 0,
				5, 6, 0, 0,
				0, 0, 0, 1);
			TMatrix4f InvA = A.Inverse();
			TMatrix4f Result = A * InvA;
			TAssert(MatrixNearlyEqual(Result, TMatrix4f::Identity(), 1e-4f));
			LOG("[PASS] Inverse: A * A^-1 == I");
		}

		// 18. Inverse of singular matrix -> identity
		{
			TMatrix4f Singular(0.0f);
			TMatrix4f InvS = Singular.Inverse();
			TAssert(MatrixNearlyEqual(InvS, TMatrix4f::Identity()));
			LOG("[PASS] Inverse of singular matrix -> identity");
		}

		// 19. TransposeAdjoint
		{
			TMatrix4f I;
			TMatrix4f TA = I.TransposeAdjoint();
			TAssert(MatrixNearlyEqual(TA, I));
			LOG("[PASS] TransposeAdjoint of identity");
		}

		// 20. TransformVector4
		{
			TMatrix4f I;
			TVector4f v(1.0f, 2.0f, 3.0f, 1.0f);
			TVector4f result = I.TransformVector4(v);
			TAssert(NearlyEqual(result.X, 1.0f) && NearlyEqual(result.Y, 2.0f) && NearlyEqual(result.Z, 3.0f) && NearlyEqual(result.W, 1.0f));
			LOG("[PASS] TransformVector4 with identity");
		}

		// 21. TransformPosition (translation)
		{
			TVector3f x(1, 0, 0);
			TVector3f y(0, 1, 0);
			TVector3f z(0, 0, 1);
			TVector3f t(10, 20, 30);
			TMatrix4f mat(x, y, z, t);
			TVector4f result = mat.TransformPosition(TVector3f(1.0f, 2.0f, 3.0f));
			TAssert(NearlyEqual(result.X, 11.0f));
			TAssert(NearlyEqual(result.Y, 22.0f));
			TAssert(NearlyEqual(result.Z, 33.0f));
			TAssert(NearlyEqual(result.W, 1.0f));
			LOG("[PASS] TransformPosition (translation applied)");
		}

		// 22. TransformDirection (translation ignored)
		{
			TVector3f x(1, 0, 0);
			TVector3f y(0, 1, 0);
			TVector3f z(0, 0, 1);
			TVector3f t(10, 20, 30);
			TMatrix4f mat(x, y, z, t);
			TVector4f result = mat.TransformDirection(TVector3f(1.0f, 2.0f, 3.0f));
			TAssert(NearlyEqual(result.X, 1.0f));
			TAssert(NearlyEqual(result.Y, 2.0f));
			TAssert(NearlyEqual(result.Z, 3.0f));
			TAssert(NearlyEqual(result.W, 0.0f));
			LOG("[PASS] TransformDirection (translation ignored)");
		}

		// 23. GetOrigin / SetOrigin
		{
			TMatrix4f mat;
			mat.SetOrigin(TVector3f(7.0f, 8.0f, 9.0f));
			TVector3f origin = mat.GetOrigin();
			TAssert(NearlyEqual(origin.X, 7.0f) && NearlyEqual(origin.Y, 8.0f) && NearlyEqual(origin.Z, 9.0f));
			LOG("[PASS] GetOrigin / SetOrigin");
		}

		// 24. RemoveTranslation
		{
			TVector3f x(1, 0, 0);
			TVector3f y(0, 1, 0);
			TVector3f z(0, 0, 1);
			TVector3f t(5, 6, 7);
			TMatrix4f mat(x, y, z, t);
			TMatrix4f noTrans = mat.RemoveTranslation();
			TVector3f origin = noTrans.GetOrigin();
			TAssert(NearlyEqual(origin.X, 0.0f) && NearlyEqual(origin.Y, 0.0f) && NearlyEqual(origin.Z, 0.0f));
			// Upper 3x3 should remain unchanged
			TAssert(NearlyEqual(noTrans.M[0][0], 1.0f) && NearlyEqual(noTrans.M[1][1], 1.0f) && NearlyEqual(noTrans.M[2][2], 1.0f));
			LOG("[PASS] RemoveTranslation");
		}

		// 25. GetScaledAxis / SetAxis
		{
			TMatrix4f mat(
				2, 0, 0, 0,
				0, 3, 0, 0,
				0, 0, 4, 0,
				0, 0, 0, 1);
			TVector3f axisX = mat.GetScaledAxis(0);
			TVector3f axisY = mat.GetScaledAxis(1);
			TVector3f axisZ = mat.GetScaledAxis(2);
			TAssert(NearlyEqual(axisX.X, 2.0f) && NearlyEqual(axisX.Y, 0.0f) && NearlyEqual(axisX.Z, 0.0f));
			TAssert(NearlyEqual(axisY.X, 0.0f) && NearlyEqual(axisY.Y, 3.0f) && NearlyEqual(axisY.Z, 0.0f));
			TAssert(NearlyEqual(axisZ.X, 0.0f) && NearlyEqual(axisZ.Y, 0.0f) && NearlyEqual(axisZ.Z, 4.0f));

			mat.SetAxis(0, TVector3f(1, 0, 0));
			TAssert(NearlyEqual(mat.M[0][0], 1.0f));
			LOG("[PASS] GetScaledAxis / SetAxis");
		}

		// 26. GetColumn / SetColumn
		{
			TMatrix4f mat(
				1, 2, 3, 4,
				5, 6, 7, 8,
				9, 10, 11, 12,
				13, 14, 15, 16);
			TVector3f col0 = mat.GetColumn(0);
			TAssert(NearlyEqual(col0.X, 1.0f) && NearlyEqual(col0.Y, 5.0f) && NearlyEqual(col0.Z, 9.0f));
			TVector3f col3 = mat.GetColumn(3);
			TAssert(NearlyEqual(col3.X, 4.0f) && NearlyEqual(col3.Y, 8.0f) && NearlyEqual(col3.Z, 12.0f));

			mat.SetColumn(0, TVector3f(100, 200, 300));
			TAssert(NearlyEqual(mat.M[0][0], 100.0f) && NearlyEqual(mat.M[1][0], 200.0f) && NearlyEqual(mat.M[2][0], 300.0f));
			LOG("[PASS] GetColumn / SetColumn");
		}

		// 27. GetScaleVector
		{
			TMatrix4f mat(
				2, 0, 0, 0,
				0, 3, 0, 0,
				0, 0, 4, 0,
				5, 6, 7, 1);
			TVector3f scale = mat.GetScaleVector();
			TAssert(NearlyEqual(scale.X, 2.0f));
			TAssert(NearlyEqual(scale.Y, 3.0f));
			TAssert(NearlyEqual(scale.Z, 4.0f));
			LOG("[PASS] GetScaleVector");
		}

		// 28. ApplyScale
		{
			TMatrix4f I;
			TMatrix4f Scaled = I.ApplyScale(5.0f);
			TAssert(NearlyEqual(Scaled.M[0][0], 5.0f));
			TAssert(NearlyEqual(Scaled.M[1][1], 5.0f));
			TAssert(NearlyEqual(Scaled.M[2][2], 5.0f));
			TAssert(NearlyEqual(Scaled.M[3][3], 1.0f));
			LOG("[PASS] ApplyScale");
		}

		// 29. ContainsNaN
		{
			TMatrix4f mat;
			TAssert(!mat.ContainsNaN());
			mat.M[1][2] = std::numeric_limits<float>::quiet_NaN();
			TAssert(mat.ContainsNaN());
			LOG("[PASS] ContainsNaN");
		}

		// 30. To3x4MatrixTranspose
		{
			TMatrix4f mat(
				1, 2, 3, 4,
				5, 6, 7, 8,
				9, 10, 11, 12,
				13, 14, 15, 16);
			float out[12];
			mat.To3x4MatrixTranspose(out);
			// Column 0: M[0][0], M[1][0], M[2][0], M[3][0]
			TAssert(NearlyEqual(out[0], 1.0f) && NearlyEqual(out[1], 5.0f) && NearlyEqual(out[2], 9.0f) && NearlyEqual(out[3], 13.0f));
			// Column 1: M[0][1], M[1][1], M[2][1], M[3][1]
			TAssert(NearlyEqual(out[4], 2.0f) && NearlyEqual(out[5], 6.0f) && NearlyEqual(out[6], 10.0f) && NearlyEqual(out[7], 14.0f));
			// Column 2: M[0][2], M[1][2], M[2][2], M[3][2]
			TAssert(NearlyEqual(out[8], 3.0f) && NearlyEqual(out[9], 7.0f) && NearlyEqual(out[10], 11.0f) && NearlyEqual(out[11], 15.0f));
			LOG("[PASS] To3x4MatrixTranspose");
		}

		// 31. Type conversion (float -> double)
		{
			TMatrix4f matF(
				1, 2, 3, 4,
				5, 6, 7, 8,
				9, 10, 11, 12,
				13, 14, 15, 16);
			TMatrix4d matD(matF);
			for (int32 i = 0; i < 4; i++)
				for (int32 j = 0; j < 4; j++)
					TAssert(std::fabs(matD.M[i][j] - (double)matF.M[i][j]) < 1e-10);
			LOG("[PASS] Type conversion float -> double");
		}

		// 32. operator*= (matrix)
		{
			TMatrix4f A(
				1, 2, 0, 0,
				0, 1, 0, 0,
				0, 0, 1, 0,
				0, 0, 0, 1);
			TMatrix4f B(
				1, 0, 0, 0,
				3, 1, 0, 0,
				0, 0, 1, 0,
				0, 0, 0, 1);
			TMatrix4f Expected = A * B;
			A *= B;
			TAssert(MatrixNearlyEqual(A, Expected));
			LOG("[PASS] operator*= (matrix)");
		}

		// 33. operator+= and operator*= (scalar)
		{
			TMatrix4f A(
				1, 2, 3, 4,
				5, 6, 7, 8,
				9, 10, 11, 12,
				13, 14, 15, 16);
			TMatrix4f B = A;
			A += B;
			B *= 2.0f;
			TAssert(MatrixNearlyEqual(A, B));
			LOG("[PASS] operator+= and operator*= (scalar)");
		}

		LOG("===== TestMatrix End: All Passed =====");
	}

	void TestLoadPackage()
	{
		TArray<GameResource*> res;
		// "/Game/123"
		// "/Game/Mesh/Cube"
		// "/Game/TestPNG"
		//if(PackageModule::ForceLoadPackage("/Game/Mesh/Cube"))
		{
			LOG("success load package, resource count: %llu", res.size());
		}
		//else
		{
			LOG("failed to load package");
		}
	}
}

int main(int argc, char* argv[])
{
	// Thunder::TestMatrix();
	///Thunder::TestFileSystem();
	//Thunder::TestImportResource();
	//Thunder::TestLoadPackage();
}

