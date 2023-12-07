// we recognize sqrt() is a very expensive operation

float Q_rsqrt( float number )
{
    long i;
    float x2, y;
    const float threehalfs = 1.5F;

    x2 = number * 0.5F;
    y = number;
    i = * ( long * ) &y;                        // evil floating point bit hack
    i = 0x5f3759df - ( i >> 1 );                // what the fuck?1
    y = * ( float * ) &i;
    y = y * ( threehalfs - ( x2 * y * y ) );    // 1st iteration
    // y = y * ( threehalfs - ( x2 * y * y ) ); // 2nd iter. (optional)

    return y;
}

