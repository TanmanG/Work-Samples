using System.Numerics;

public enum Axis
{
    X,  
    Y,
    Z
}

public static class VertexPipeline
{
    // PERSPECTIVE RENDERING
    public static Matrix4x4 GetPerspectiveMatrix(float aspectRatio, float fov,
                                               float nearClippingDistance,
                                               float farClippingDistance)
    {
        fov *= MathF.PI / 180;

        if (nearClippingDistance <= 0)
            throw new Exception("Near clipping distance must be non-zero and positive!");
        if (farClippingDistance <= 0)
            throw new Exception("Far clipping distance must be non-zero and positive!");
        if (fov is <= 0 or >= 180)
            throw new Exception("FOV must be between 0 and 180!");
        if (aspectRatio <= 0)
            throw new Exception("Aspect Ratio must be non-zero and positive!");

        Matrix4x4 perspectiveMatrix = new()
        {
            M11 = 1 / (aspectRatio * MathF.Tan(fov / 2)),
            M22 = 1 / MathF.Tan(fov / 2),
            M33 = -(farClippingDistance + nearClippingDistance) / (farClippingDistance - nearClippingDistance),
            M34 = -2 * nearClippingDistance * farClippingDistance / (farClippingDistance - nearClippingDistance),
            M43 = -1,
        };

        return perspectiveMatrix;
    }
    public static float[,] ComputeVertexNDCs(List<Vector3> vertices, Matrix4x4 perspectiveMatrix, Vector3 position, Vector3 front, Vector3 right)
    {
        Vector3 up = Vector3.Cross(front, right);
        perspectiveMatrix *= new Matrix4x4()
        {
            M11 = right.X,
            M12 = right.Y,
            M13 = right.Z,
            M14 = -Vector3.Dot(right, position),
            M21 = up.X,
            M22 = up.Y,
            M23 = up.Z,
            M24 = -Vector3.Dot(up, position),
            M31 = -front.X,
            M32 = -front.Y,
            M33 = -front.Z,
            M34 = Vector3.Dot(front, position),
            M41 = 0,
            M42 = 0,
            M43 = 0,
            M44 = 1,
        };
        List<Vector3> processedVectors = GetNdcCoordinates(
                                            GetHomogenizedVectors(
                                                inputVectors: vertices,
                                                perspectiveMatrix: perspectiveMatrix));

        return Vector3ListToArray(processedVectors);
    }
    public static List<Vector4> GetHomogenizedVectors(List<Vector3> inputVectors, Matrix4x4 perspectiveMatrix)
    {
        List<Vector4> homogenizedVectors = new();

        foreach (Vector3 vector in inputVectors)
        {
            homogenizedVectors.Add(MultiplyVecMatrix4(vector: new Vector4(vector, 1), matrix: perspectiveMatrix));
        }

        return homogenizedVectors;
    }
    public static List<Vector3> GetNdcCoordinates(List<Vector4> inputVectors)
    {
        List<Vector3> homogenizedVectors = new();

        // For each homogenous coordinate, apply NDC divide to place it into NDC space
        foreach (Vector4 vector in inputVectors)
        {
            homogenizedVectors.Add(Vector4To3(vector, 1 / vector.W));
        }

        return homogenizedVectors;
    }
    public static List<Tuple<int, int>> GetScreenCoordinates(List<Vector3> inputVectors, int viewportWidth, int viewportHeight)
    {
        Dictionary<Tuple<int, int>, float> screenCoordinates = new();

        foreach (Vector3 vector in inputVectors)
        {
            Tuple<int, int> newPixel = new((int)((vector.X + 1) / 2 * viewportWidth),
                                           (int)((vector.Y + 1) / 2 * viewportHeight));

            // Check if the new pixel is closer than the old one
            if (!screenCoordinates.TryGetValue(newPixel, out float value) || vector.Z < value)
            {
                // Replace/Add the pixel
                screenCoordinates.Remove(newPixel);
                screenCoordinates.Add(newPixel, vector.Z);
            }
        }

        return screenCoordinates.Keys.ToList();
    }

    // HELPER FUNCTIONS
    public static Vector3 Vector4To3(Vector4 inputVector, float scalar = 1)
    {
        return new Vector3(x: inputVector.X * scalar, y: inputVector.Y * scalar, z: inputVector.Z * scalar);
    }
    public static float[,] Vector3ListToArray(List<Vector3> inputVectors)
    {
        float[,] vectorArray = new float[inputVectors.Count, 3];

        for (int row = 0; row < inputVectors.Count; row++)
        {
            vectorArray[row, 0] = inputVectors[row].X;
            vectorArray[row, 1] = inputVectors[row].Y;
            vectorArray[row, 2] = inputVectors[row].Z;
        }

        return vectorArray;
    }

    // OPERATION FUNCTIONS
    public static List<Vector4> ApplyCameraRotationAndPosition(List<Vector3> inputVectors, Vector3 position, Vector3 front, Vector3 up)
    {
        List<Vector4> rotatedVectors = new();

        Matrix4x4 viewMatrix = Matrix4x4.CreateLookAt(cameraPosition: position, cameraTarget: position + front, cameraUpVector: up);

        foreach (Vector3 vector in inputVectors)
        {
            rotatedVectors.Add(MultiplyVecMatrix4(new Vector4(vector, w: 1), matrix: viewMatrix));
        }

        return rotatedVectors;
    }
    public static Vector4 MultiplyVecMatrix4(Vector4 vector, Matrix4x4 matrix)
    {
        return new Vector4
        {
            X = vector.X * matrix.M11 + vector.Y * matrix.M12 + vector.Z * matrix.M13 + vector.W * matrix.M14,
            Y = vector.X * matrix.M21 + vector.Y * matrix.M22 + vector.Z * matrix.M23 + vector.W * matrix.M24,
            Z = vector.X * matrix.M31 + vector.Y * matrix.M32 + vector.Z * matrix.M33 + vector.W * matrix.M34,
            W = vector.X * matrix.M41 + vector.Y * matrix.M42 + vector.Z * matrix.M43 + vector.W * matrix.M44,
        };
    }
    public static List<Vector3> RotateVectorsAboutAxis(List<Vector3> vectors, Axis axis, float degrees, bool usingRadians = false)
    {
	    // Converts degrees to radians
	    if (!usingRadians)
		    degrees *= MathF.PI / 180;
	    
	    // Compute sin & cos
	    var sinCos = MathF.SinCos(degrees);
	    
	    // Create a list to hold the rotated vectors
        List<Vector3> rotatedVectors = new();
        // Rotate each vector
        foreach (Vector3 vector in vectors)
        {
            // Store the rotated vector
            rotatedVectors.Add(_RotateVectorAboutAxis(vector, axis, sin: sinCos.Sin, cos: sinCos.Cos));
        }
        
        return rotatedVectors;
    }
    public static Vector3 RotateVectorAboutAxis(Vector3 vector, Axis axis, float degrees, bool usingRadians = false)
    {
	    // Converts degrees to radians
	    if (!usingRadians)
		    degrees *= MathF.PI / 180;
	    
	    // Compute sin & cos
	    var sinCos = MathF.SinCos(degrees);

	    return _RotateVectorAboutAxis(vector, axis, sin: sinCos.Sin, cos: sinCos.Cos);
    }
    private static Vector3 _RotateVectorAboutAxis(Vector3 vector, Axis axis, float sin, float cos)
    {
        Vector3 rotatedVector = new();

        // Branches to which axis is selected
        switch (axis)
        {
            case Axis.X: // Rotate about X-axis
	            rotatedVector.X = vector.X;
	            rotatedVector.Y = vector.Y * cos - vector.Z * sin;
	            rotatedVector.Z = vector.Y * sin + vector.Z * cos;
                break;

            case Axis.Y: // Rotate about Y-axis
	            rotatedVector.X = vector.X * cos + vector.Z * sin;
	            rotatedVector.Y = vector.Y;
	            rotatedVector.Z = -vector.X * sin + vector.Z * cos;
                break;

            case Axis.Z: // Rotate about Z-axis
	            rotatedVector.X = vector.X * cos - vector.Y * sin;
	            rotatedVector.Y = vector.X * sin + vector.Y * cos;
	            rotatedVector.Z = vector.Z;
                break;
        }
        
        return rotatedVector;
    }
}

