// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2017 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#ifndef GU_GJKRAYCAST_H
#define GU_GJKRAYCAST_H

#include "GuGJKType.h"
#include "GuGJKSimplex.h"
#include "GuConvexSupportTable.h"
#include "GuGJKPenetration.h"
#include "GuEPA.h"


namespace physx
{


namespace Gu
{

	/*
		ConvexA is in the local space of ConvexB
		lambda			:	the time of impact(TOI)
		initialLambda	:	the start time of impact value (disable)
		s				:	the sweep ray origin
		r				:	the normalized sweep ray direction scaled by the sweep distance. r should be in ConvexB's space
		normal			:	the contact normal
		inflation		:	the amount by which we inflate the swept shape. If the inflated shapes aren't initially-touching, 
							the TOI will return the time at which both shapes are at a distance equal to inflation separated. If inflation is 0
							the TOI will return the time at which both shapes are touching.
	
	*/
	template<class ConvexA, class ConvexB>
	bool gjkRaycast(const ConvexA& a, const ConvexB& b, const Ps::aos::Vec3VArg initialDir, const Ps::aos::FloatVArg initialLambda, const Ps::aos::Vec3VArg s, const Ps::aos::Vec3VArg r, Ps::aos::FloatV& lambda, Ps::aos::Vec3V& normal, Ps::aos::Vec3V& closestA, const PxReal _inflation)
	{
		PX_UNUSED(initialLambda);

		using namespace Ps::aos;

		const FloatV inflation = FLoad(_inflation);
		const Vec3V zeroV = V3Zero();
		const FloatV zero = FZero();
		const FloatV one = FOne();
		const BoolV bTrue = BTTTT();

		const FloatV maxDist = FLoad(PX_MAX_REAL);
	
		FloatV _lambda = zero;//initialLambda;
		Vec3V x = V3ScaleAdd(r, _lambda, s);
		PxU32 size=1;
		
		//const Vec3V dir = V3Sub(a.getCenter(), b.getCenter());
		const Vec3V _initialSearchDir = V3Sel(FIsGrtr(V3Dot(initialDir, initialDir), FEps()), initialDir, V3UnitX());
		const Vec3V initialSearchDir = V3Normalize(_initialSearchDir);

		const Vec3V initialSupportA(a.ConvexA::support(V3Neg(initialSearchDir)));
		const Vec3V initialSupportB( b.ConvexB::support(initialSearchDir));
		 
		Vec3V Q[4] = {V3Sub(initialSupportA, initialSupportB), zeroV, zeroV, zeroV}; //simplex set
		Vec3V A[4] = {initialSupportA, zeroV, zeroV, zeroV}; //ConvexHull a simplex set
		Vec3V B[4] = {initialSupportB, zeroV, zeroV, zeroV}; //ConvexHull b simplex set
		 

		Vec3V v = V3Neg(Q[0]);
		Vec3V supportA = initialSupportA;
		Vec3V supportB = initialSupportB;
		Vec3V support = Q[0];

		const FloatV minMargin = FMin(a.ConvexA::getSweepMargin(), b.ConvexB::getSweepMargin());
		const FloatV eps1 = FMul(minMargin, FLoad(0.1f));
		const FloatV inflationPlusEps(FAdd(eps1, inflation));
		const FloatV eps2 = FMul(eps1, eps1);

		const FloatV inflation2 = FMul(inflationPlusEps, inflationPlusEps);

		Vec3V clos(Q[0]);
		Vec3V preClos = clos;
		//Vec3V closA(initialSupportA);
		FloatV sDist = V3Dot(v, v);
		FloatV minDist = sDist;
		//Vec3V closAA = initialSupportA;
		//Vec3V closBB = initialSupportB;
		
		BoolV bNotTerminated = FIsGrtr(sDist, eps2);
		BoolV bNotDegenerated = bTrue;

		Vec3V nor = v;
		
		while(BAllEqTTTT(bNotTerminated))
		{
			
			minDist = sDist;
			preClos = clos;

			const Vec3V vNorm = V3Normalize(v);
			const Vec3V nvNorm = V3Neg(vNorm);

			supportA=a.ConvexA::support(vNorm);
			supportB=V3Add(x, b.ConvexB::support(nvNorm));
		
			//calculate the support point
			support = V3Sub(supportA, supportB);
			const Vec3V w = V3Neg(support);
			const FloatV vw = FSub(V3Dot(vNorm, w), inflationPlusEps);
			if(FAllGrtr(vw, zero))
			{
				const FloatV vr = V3Dot(vNorm, r);
				if(FAllGrtrOrEq(vr, zero))
				{
					return false;
				}
				else
				{
					const FloatV _oldLambda = _lambda;
					_lambda = FSub(_lambda, FDiv(vw, vr));
					if(FAllGrtr(_lambda, _oldLambda))
					{
						if(FAllGrtr(_lambda, one))
						{
							return false;
						}
						const Vec3V bPreCenter = x;
						x = V3ScaleAdd(r, _lambda, s);
						
						const Vec3V offSet =V3Sub(x, bPreCenter);
						const Vec3V b0 = V3Add(B[0], offSet);
						const Vec3V b1 = V3Add(B[1], offSet);
						const Vec3V b2 = V3Add(B[2], offSet);
					
						B[0] = b0;
						B[1] = b1;
						B[2] = b2;

						Q[0]=V3Sub(A[0], b0);
						Q[1]=V3Sub(A[1], b1);
						Q[2]=V3Sub(A[2], b2);

						supportB = V3Add(x, b.ConvexB::support(nvNorm));
						support = V3Sub(supportA, supportB);
						minDist = maxDist;
						nor = v;
						//size=0;
					}
				}
			}

			PX_ASSERT(size < 4);
			A[size]=supportA;
			B[size]=supportB;
			Q[size++]=support;
	
			//calculate the closest point between two convex hull
			clos = GJKCPairDoSimplex(Q, A, B, support, size);
			v = V3Neg(clos);
			sDist = V3Dot(clos, clos);
			
			bNotDegenerated = FIsGrtr(minDist, sDist);
			bNotTerminated = BAnd(FIsGrtr(sDist, inflation2), bNotDegenerated);
		}

		
		const BoolV aQuadratic = a.isMarginEqRadius();
		//ML:if the Minkowski sum of two objects are too close to the original(eps2 > sDist), we can't take v because we will lose lots of precision. Therefore, we will take
		//previous configuration's normal which should give us a reasonable approximation. This effectively means that, when we do a sweep with inflation, we always keep v because
		//the shapes converge separated. If we do a sweep without inflation, we will usually use the previous configuration's normal.
		nor = V3Sel(BAnd(FIsGrtr(sDist, eps2), bNotDegenerated), v, nor);
		nor = V3Neg(V3NormalizeSafe(nor, V3Zero()));
		normal = nor;
		lambda = _lambda;
		
		const Vec3V closestP = V3Sel(bNotDegenerated, clos, preClos);
		Vec3V closA = zeroV, closB = zeroV;
		getClosestPoint(Q, A, B, closestP, closA, closB, size);
		closestA = V3Sel(aQuadratic, V3NegScaleSub(nor, a.getMargin(), closA), closA);  
		
		return true;
	}



	/*
		ConvexA is in the local space of ConvexB
		lambda			:	the time of impact(TOI)
		initialLambda	:	the start time of impact value (disable)
		s				:	the sweep ray origin in ConvexB's space
		r				:	the normalized sweep ray direction scaled by the sweep distance. r should be in ConvexB's space
		normal			:	the contact normal in ConvexB's space
		closestA		:	the tounching contact in ConvexB's space
		inflation		:	the amount by which we inflate the swept shape. If the inflated shapes aren't initially-touching, 
							the TOI will return the time at which both shapes are at a distance equal to inflation separated. If inflation is 0
							the TOI will return the time at which both shapes are touching.
	
	*/
	template<class ConvexA, class ConvexB>
	bool gjkRaycastPenetration(const ConvexA& a, const ConvexB& b, const Ps::aos::Vec3VArg initialDir, const Ps::aos::FloatVArg initialLambda, const Ps::aos::Vec3VArg s, const Ps::aos::Vec3VArg r, Ps::aos::FloatV& lambda, 
		Ps::aos::Vec3V& normal, Ps::aos::Vec3V& closestA, const PxReal _inflation, const bool initialOverlap)
	{
		using namespace Ps::aos;
		Vec3V closA;
		Vec3V norm; 
		FloatV _lambda;
		if(gjkRaycast(a, b, initialDir, initialLambda, s, r, _lambda, norm, closA, _inflation))
		{
			const FloatV zero = FZero();
			lambda = _lambda;
			if(FAllEq(_lambda, zero) && initialOverlap)
			{

				//time of impact is zero, the sweep shape is intesect, use epa to get the normal and contact point
				const FloatV contactDist = getSweepContactEps(a.getMargin(), b.getMargin());

				Vec3V closAA;
				Vec3V closBB;
				FloatV sDist;
				PxU8 aIndices[4];
				PxU8 bIndices[4];
				PxU8 size=0;

				//PX_COMPILE_TIME_ASSERT(typename Shrink<ConvexB>::Type != Gu::BoxV);
				
				typename ConvexA::ShrunkConvex convexA = a.getShrunkConvex();
				typename ConvexB::ShrunkConvex convexB = b.getShrunkConvex();


				GjkStatus status = gjkPenetration<typename ConvexA::ShrunkConvex, typename ConvexB::ShrunkConvex>(convexA, convexB, 
					initialDir, contactDist, closAA, closBB, norm, sDist, aIndices, bIndices, size, false);
				//norm = V3Neg(norm);
				if(status == GJK_CONTACT)
				{
					closA = closAA;//V3Sel(aQuadratic, V3Add(closAA, V3Scale(norm, a.getMargin())),closAA);
				}
				else if(status == EPA_CONTACT)
				{
					status = epaPenetration(a, b, aIndices, bIndices, size, closAA, closBB, norm, sDist);

					if (status == EPA_CONTACT || status == EPA_DEGENERATE)
					{
						closA = closAA;
					}
					else
					{
						//ML: if EPA fail, we will use the ray direction as the normal and set pentration to be zero
						norm = V3Normalize(V3Neg(r));
						closA = V3Zero();
						sDist = zero;//FNeg(V3Length(V3Sub(closBB, closAA)));
					}
				}
				else
				{
					//ML:: this will be gjk degenerate case, we will take the normal and sDist from the gjkRelativePenetration
					closA = closAA;
				}
				lambda = FMin(zero, sDist);
			}
			closestA = closA;
			normal = norm;
			return true;
		}
		return false;
	}
}
}

#endif
