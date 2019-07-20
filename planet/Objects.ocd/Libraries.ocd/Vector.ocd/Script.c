/**
 Library for vector operations.
 
 @author Marky
 @credits Randrian, since many functions have their origin in the rope physics library.
 */


/**
 Addition of two vectors of the same dimension.
 Note: Cannot be named 'Add' because the scenario
 save mechanism will throw a lot of warnings.
 @par a The first vector.
 @par b The second vector.
 @return array The vector (a_i + b_i), i = 1,...,n.
*/
public func Sum(array a, array b)
{
	AssertVectorOperation(a, b);

	var vector = [];
	
	for (var i = 0; i < GetLength(a); i++)
	{
		vector[i] = a[i] + b[i];
	}

	return vector;
}


/**
 Subtraction of two vectors of the same dimension.
 @par a The first vector.
 @par b The second vector.
 @return array The vector (a_i - b_i), i = 1,...,n.
*/
public func Subtract(array a, array b)
{
	AssertVectorOperation(a, b);

	var vector = [];

	for (var i = 0; i < GetLength(a); i++)
	{
		vector[i] = a[i] - b[i];
	}

	return vector;
}


/**
 Multiplication of a vector and a number.
 @par a The vector
 @par b The number
 @return array The vector (b * a_i), i = 1,...,n.
*/
public func Multiply(array a, int b)
{
	AssertNotNil(a, "vector 'a'");

	var vector = [];

	for (var i = 0; i < GetLength(a); i++)
	{
		vector[i] = b * a[i];
	}

	return vector;
}


/**
 Division of a vector and a number.
 @par a The vector
 @par b The number
 @return array The vector (a_i / b), i = 1,...,n.
*/
public func Divide(array a, int b)
{
	AssertNotNil(a, "vector 'a'");

	var vector = [];

	for (var i = 0; i < GetLength(a); i++)
	{
		vector[i] = a[i] / b;
	}

	return vector;
}


/**
 Dot product of two vectors of the same dimension.
 @par a The first vector
 @par b The second vector
 @return int Sum of a_i * b_i, for i = 1,...,n
*/
public func DotProduct(array a, array b)
{
	AssertVectorOperation(a, b);

	var sum = 0;

	for (var i = 0; i < GetLength(a); i++)
	{
		sum += a[i] * b[i];
	}

	return sum;
}

/**
 Product of the elements of vectors of the same dimension.
 @par a The first vector
 @par b The second vector
 @return array the vector c_i of c_i = a_i * b_i, for i = 1,...,n
*/
public func Product(array a, array b)
{
	AssertVectorOperation(a, b);

	var vector = [];

	for (var i = 0; i < GetLength(a); i++)
	{
		vector[i] = a[i] * b[i];
	}

	return vector;
}

/**
 Euclidean length of a vector.
 @par a The vector.
 @return int Square root of the sum of a_i * a_i, for i = 1,...,n.
*/
public func Length(array a)
{
	return Sqrt(DotProduct(a, a));
}


/**
 The dimension of a vector.
 @par a The vector
 @return int The number of elements of the vector.
 */
public func Dimension(array a)
{
	AssertNotNil(a, "vector 'a'");

	return GetLength(a);
}


/**
 Angle between two vectors.
 @par a A two-dimensional vector.
 @par b A two-dimensional vector.
 @return int The angle between a and b. If you rotate a by this angle,
             then it will point in the same direction as b.
*/
public func AngleBetween(array a, array b, int precision)
{
	AssertVectorOperation(a, b);
	if (!precision) precision = 1;

	var rot_a = GetRotation(a, precision);
	var rot_b = GetRotation(b, precision);
	return rot_b - rot_a;
}


/**
 Angle of a vector.
 @par a A two-dimensional vector.
 @par precision [opt] Multiplied with the angle, for higher precision. A precision of 10 will produce values from 0 to 3600.
 @return int The angle of a, relative to the coordinate system.
*/
public func GetRotation(array a, int precision)
{
	AssertDimension(a, 2);
	if (!precision) precision = 1;

	return Angle(0, 0, a[0], a[1], precision);
}


/**
 Normalizes a vector with precision.
 @par a The vector.
 @par precision Factor for the resultion length.
 @return array The normalized vector with length = 1 * precision
*/
public func Normalize(array a, int precision)
{
	if (!precision) precision = 1;
	return Divide(Multiply(a, precision), Length(a));
}


/**
 Rotates a vector by an angle.
 @par a The vector.
 @par angle The angle that the vector is rotated by, clockwise.
 @return array The rotated vector.
 */
public func Rotate(array a, int angle, int precision)
{
	AssertDimension(a, 2);
	if (!precision) precision = 1;
	var length_precision = 1000;

	var cos = Cos(angle, length_precision, precision);
	var sin = Sin(angle, length_precision, precision);

	var rotated_x = cos * a[0] - sin * a[1];
	var rotated_y = sin * a[0] + cos * a[1];

	return [rotated_x / length_precision, rotated_y / length_precision];
}


/**
 Assert that a vector operation can be done with these two vectors.
 The vectors have to be of the same dimension and their elements
 must have the same names.
 @par a The first vector, must not be nil.
 @par b The second vector, must not be nil.
 */
public func AssertVectorOperation(array a, array b)
{
	AssertNotNil(a, "vector 'a'");
	AssertNotNil(b, "vector 'b'");

	var length_a = GetLength(a);
	var length_b = GetLength(b);
	if (length_a != length_b)
	{
		FatalError(Format("This method only works with vectors of the same length. Vector 'a' has %d elements, vector 'b' has %d elements", length_a, length_b));
	}
}


/**
 Asserts that a vector has a specific dimesion.
 @par a The vector.
 @par dimension The dimension.
 */
public func AssertDimension(array a, int dimension)
{
	var dim = Dimension(a); 
	if (dim != dimension)
	{
		FatalError(Format("The function expects a vector of length %d, got length %d", dimension, dim));
	}
}

/**
 Asserts that a parameter is not nil.
 @par parameter The parameter that is checked.
 @par message [optional] a descriptive message for the parameter that is printed in the fatal error.
 */
public func AssertNotNil(parameter, string message)
{
	if (parameter == nil)
	{
		if (!message) message = "parameter";
		
		FatalError(Format("Expected %s that is not nil", message));
	}
}