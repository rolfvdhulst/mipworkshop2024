//
// Created by rolf on 1-8-23.
//

#include "mipworkshop2024/Shared.h"
#include <cassert>
#include <array>

std::optional<Rat64> realToRational(double value,
                                    double minDelta,
                                    double maxDelta,
                                    long int maxDenom) {
    assert(minDelta < 0.0);
    assert(maxDelta > 0.0);

    if (fabs(value) >= ((double) std::numeric_limits<long int>::max()) / (double) maxDenom) {
        return std::nullopt;
    }

    constexpr std::array<double, 20> simpleDenoms = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 11.0, 12.0, 13.0,
                                                     14.0, 15.0, 16.0,
                                                     17.0, 18.0, 19.0, 25.0, -1.0};

    for (const auto &simpleDenom: simpleDenoms) {
        double denom = simpleDenom;
        while (denom <= maxDenom) {
            double nom = floor(value * denom);
            double ratval0 = nom / denom;
            double ratval1 = (nom + 1.0) / denom;
            if (minDelta <= value - ratval0 && value - ratval1 <= maxDelta) {
                if (value - ratval0 <= maxDelta) {
                    return Rat64{.nominator = (long int) nom, .denominator = (long int) denom};
                }
                if (minDelta <= value - ratval1) {
                    return Rat64{.nominator = (long int) (nom + 1.0), .denominator = (long int) denom};

                }
            }
            denom *= 10.0;
        }
    }

    /* the simple denominators didn't work: calculate rational representation with arbitrary denominator */

    double epsilon = fmin(-minDelta, maxDelta) / 2.0;
    double b = value;
    double a = floor(b + epsilon);
    double g0 = a;
    double h0 = 1.0;
    double g1 = 1.0;
    double h1 = 0.0;
    double delta0 = value - a; //a = g0/h0
    double delta1 = (delta0 < 0.0 ? value - (a - 1.0) : value - (a + 1.0));

    while ((delta0 < minDelta || delta0 > maxDelta) && (delta1 < minDelta || delta1 > maxDelta)) {
        assert(b > a + epsilon);
        assert(h0 >= 0.0);
        assert(h1 >= 0.0);

        b = 1.0 / (b - a);
        a = floor(b + epsilon);
        double gx = g0;
        double hx = h0;

        g0 = a * g0 + g1;
        h0 = a * h0 + h1;

        g1 = gx;
        h1 = hx;
        if (h0 > maxDenom) {
            return std::nullopt;
        }
        delta0 = value - g0 / h0;
        delta1 = (delta0 < 0.0 ? value - (g0 - 1.0) / h0 : value - (g0 + 1.0) / h0);
    }
    double limit = ((double) std::numeric_limits<long int>::max()) / 16.0;

    if (fabs(g0) > limit || h0 > limit) {
        return std::nullopt;
    }
    assert(h0 > 0.5);
    if (delta0 < minDelta) {
        assert(minDelta < delta1 && delta1 <= maxDelta);
        return Rat64{.nominator = (long int) (g0 - 1.0), .denominator = (long int) h0};
    } else if (delta0 > maxDelta) {
        assert(minDelta < delta1 && delta1 <= maxDelta);
        return Rat64{.nominator = (long int) (g0 + 1.0), .denominator = (long int) h0};
    }
    return Rat64{.nominator = (long int) (g0), .denominator = (long int) h0};
}

double relDiff(
        double             val1,               /**< first value to be compared */
        double             val2                /**< second value to be compared */
)
{
    double absval1;
    double absval2;
    double quot;

    absval1 = fabs(val1);
    absval2 = fabs(val2);
    quot = std::max(1.0, std::max(absval1, absval2));

    return (val1-val2)/quot;
}

bool isIntegralScalar(
        double val,                /**< value that should be scaled to an integral value */
        double scalar,             /**< scalar that should be tried */
        double mindelta,           /**< minimal relative allowed difference of scaled coefficient s*c and integral i */
        double maxdelta            /**< maximal relative allowed difference of scaled coefficient s*c and integral i */
)
{
    double sval;
    double downval;
    double upval;

    assert(mindelta <= 0.0);
    assert(maxdelta >= 0.0);

    sval = val * scalar;
    downval = floor(sval);
    upval = ceil(sval);

    return (relDiff(sval, downval) <= maxdelta || relDiff(sval, upval) >= mindelta);
}

/** calculates the greatest common divisor of the two given values */
long int calculateGCD(
        long int val1,               /**< first value of greatest common devisor calculation */
        long int val2                /**< second value of greatest common devisor calculation */
)
{
    int t;

    assert(val1 > 0);
    assert(val2 > 0);

    t = 0;
    /* if val1 is even, divide it by 2 */
    while( !(val1 & 1) )
    {
        val1 >>= 1; /*lint !e704*/

        /* if val2 is even too, divide it by 2 and increase t(=number of e) */
        if( !(val2 & 1) )
        {
            val2 >>= 1; /*lint !e704*/
            ++t;
        }
            /* only val1 can be odd */
        else
        {
            /* while val1 is even, divide it by 2 */
            while( !(val1 & 1) )
                val1 >>= 1; /*lint !e704*/

            break;
        }
    }

    /* while val2 is even, divide it by 2 */
    while( !(val2 & 1) )
        val2 >>= 1; /*lint !e704*/

    /* the following if/else condition is only to make sure that we do not overflow when adding up both values before
     * dividing them by 4 in the following while loop
     */
    if( t == 0 )
    {
        if( val1 > val2 )
        {
            val1 -= val2;

            /* divide val1 by 2 as long as possible  */
            while( !(val1 & 1) )
                val1 >>= 1;   /*lint !e704*/
        }
        else if( val1 < val2 )
        {
            val2 -= val1;

            /* divide val2 by 2 as long as possible  */
            while( !(val2 & 1) )
                val2 >>= 1;   /*lint !e704*/
        }
    }

    /* val1 and val2 are odd */
    while( val1 != val2 )
    {
        if( val1 > val2 )
        {
            /* we can stop if one value reached one */
            if( val2 == 1 )
                return (val2 << t);  /*lint !e647 !e703*/

            /* if ((val1 xor val2) and 2) = 2, then gcd(val1, val2) = gcd((val1 + val2)/4, val2),
             * and otherwise                        gcd(val1, val2) = gcd((val1 − val2)/4, val2)
             */
            if( ((val1 ^ val2) & 2) == 2 )
                val1 += val2;
            else
                val1 -= val2;

            assert((val1 & 3) == 0);
            val1 >>= 2;   /*lint !e704*/

            /* if val1 is still even, divide it by 2  */
            while( !(val1 & 1) )
                val1 >>= 1;   /*lint !e704*/
        }
        else
        {
            /* we can stop if one value reached one */
            if( val1 == 1 )
                return (val1 << t);  /*lint !e647 !e703*/

            /* if ((val2 xor val1) and 2) = 2, then gcd(val2, val1) = gcd((val2 + val1)/4, val1),
             * and otherwise                        gcd(val2, val1) = gcd((val2 − val1)/4, val1)
             */
            if( ((val2 ^ val1) & 2) == 2 )
                val2 += val1;
            else
                val2 -= val1;

            assert((val2 & 3) == 0);
            val2 >>= 2;   /*lint !e704*/

            /* if val2 is still even, divide it by 2  */
            while( !(val2 & 1) )
                val2 >>= 1;   /*lint !e704*/
        }
    }

    return (val1 << t);  /*lint !e703*/
}

std::optional<double> calculateIntegralScalar(const std::vector<double> &values,
                                              double minDelta,
                                              double maxDelta,
                                              long int maxDenom,
                                              double maxScale) {
    double minVal = std::numeric_limits<double>::max();
    for(const auto& val : values){
        if(val < minDelta || val > maxDelta){
            minVal = fmin(minVal,fabs(val));
        }
    }
    if(minVal == std::numeric_limits<double>::max()){
        return 1.0; //All coefficients are zero, inside tolerances
    }

    assert(minVal > fmin(-minDelta, maxDelta));
    constexpr std::array<double,9> scalars =  {3.0,5.0,7.0,9.0,11.0,13.0,15.0,17.0,19.0};

    constexpr double INVALID_REAL = 1e99;
    double bestScalar = INVALID_REAL;
    for (int i = 0; i < 2; ++i) {
        double scaleVal = i == 0 ? 1.0 /minVal : 1.0;
        bool scalable = true;
        for (int c = 0; c < values.size() && scalable; ++c) {
            double val = values[c];
            if(val == 0.0) continue;
            double absVal = fabs(val);
            while(scaleVal <= maxScale && (absVal * scaleVal < 0.5 || !isIntegralScalar(val,scaleVal,minDelta,maxDelta))) {
                bool scaled = false;
                for(double scalar : scalars){
                    if(isIntegralScalar(val,scaleVal* scalar,minDelta,maxDelta)){
                        scaleVal *= scalar;
                        scaled = true;
                        break;
                    }
                }
                if(!scaled){
                    scaleVal *= 2.0;
                }
            }
            scalable = (scaleVal <= maxScale);
        }
        if(scalable){
            if(scaleVal < bestScalar){
                bestScalar = scaleVal;
            }

            if (i == 0 && isFeasEq(scaleVal,1.0/minVal) ) {
                return bestScalar;
            }
        }
    }


    /* convert each value into a rational number, calculate the greatest common divisor of the nominators
   * and the smallest common multiple of the denominators
   */
    long int gcd = 1;
    long int scm = 1;
    bool rational = true;
    int i;
    for (i = 0; i < values.size() && rational; ++i) {
        double value = values[i];
        if(value == 0.0) continue;
        auto rat = realToRational(value,minDelta,maxDelta,maxDenom);
        if(!rat.has_value()){
            return std::nullopt;
        }
        if(rat->nominator == 0) continue;
        gcd = std::abs(rat->nominator);
        scm = std::abs(rat->denominator);
        rational = ((double) scm / (double) gcd )<= maxScale;
        break;
    }
    for (++i;  i < values.size() && rational; ++i) {
        double val = values[i];
        if (val == 0.0) continue;
        auto rat = realToRational(val,minDelta,maxDelta,maxDenom);
        if(!rat.has_value()){
            return std::nullopt;
        }
        if(rat->nominator == 0) continue;
        if(rat->denominator < 0){
            rat->nominator *= -1;
            rat->denominator *= -1;
        }
        assert(rat->denominator > 0);
        gcd = calculateGCD(gcd,std::abs(rat->nominator));
        scm *= rat->denominator / calculateGCD(std::abs(scm),rat->denominator);
        rational = ((double) scm / (double) gcd )<= maxScale;
    }
    if(rational){
        assert(((double) scm / (double) gcd )<= maxScale);
        if(((double) scm / (double) gcd ) < bestScalar){
            bestScalar = (double) scm / (double) gcd;
        }
    }
    if(bestScalar < INVALID_REAL){
        return bestScalar;
    }
    return std::nullopt;

}

