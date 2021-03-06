/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Application
    potentialFoam

Description
    Simple potential flow solver which can be used to generate starting fields
    for full Navier-Stokes codes.

\*---------------------------------------------------------------------------*/

#include "fvCFD.H"
#include "OFstream.H"

#include <armadillo>
#include "complex.H"

#include <math.h>
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
    argList::addBoolOption("writep", "write the final pressure field");
    argList::addBoolOption
    (
        "initialiseUBCs",
        "initialise U boundary conditions"
    );

    #include "setRootCase.H"
    #include "createTime.H"
    #include "createMesh.H"
    #include "readControls.H"
    #include "createFields.H"

    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    Info<< nl << "Calculating potential flow" << endl;

    // Since solver contains no time loop it would never execute
    // function objects so do it ourselves.
    runTime.functionObjects().start();

    adjustPhi(phi, U, p);

    for (int nonOrth=0; nonOrth<=nNonOrthCorr; nonOrth++)
    {
        fvScalarMatrix pEqn
        (
            fvm::laplacian
            (
                dimensionedScalar
                (
                    "1",
                    dimTime/p.dimensions()*dimensionSet(0, 2, -2, 0, 0),
                    1
                ),
                p
            )
         ==
            fvc::div(phi)
        );

        pEqn.setReference(pRefCell, pRefValue);
//         pEqn.solve();
	
//-----------------------------------------------               
//                List<List<scalar> > A; 
//                List<scalar> b;
                
                double upSum = 0;			//
                double downSum = 0;			//
                double F = 0;				//
                

                arma::mat Ar = arma::zeros<arma::mat>(p.size(),p.size());
		arma::mat br = arma::zeros<arma::mat>(1,p.size());
// 		upSum = p.size();
                forAll(p,i) // forAll(A,i)
                {
//            	    A[i][i] = pEqn.diag()[i];
            	    Ar(i,i) = pEqn.diag()[i];
//            	    b[i]    = pEqn.source()[i];
		    br(i) = pEqn.source()[i];
		  
		}
                
                const lduAddressing& addr = pEqn.lduAddr();
                const labelList& lowerAddr = addr.lowerAddr();
                const labelList& upperAddr = addr.upperAddr();                
                
                forAll(lowerAddr, i)
                {
		    Ar(lowerAddr[i],upperAddr[i]) = pEqn.upper()[i];
		    Ar(upperAddr[i],lowerAddr[i]) = pEqn.lower()[i];
//            	    A[lowerAddr[i]][upperAddr[i]] = pEqn.upper()[i];
//            	    A[upperAddr[i]][lowerAddr[i]] = pEqn.lower()[i];           	                	    
// // //             	    downSum += pEqn.upper()[i]* pEqn.upper()[i];
////////            	    downSum += pEqn.lower()[i]*pEqn.lower()[i];    
                }
                
                forAll(p.boundaryField(),I)
                {
            	    const fvPatch &ptch=p.boundaryField()[I].patch();
            	    forAll(ptch,J)
            	    {
           		int w=ptch.faceCells()[J];
//            		A[w][w]+=pEqn.internalCoeffs()[I][J];
			Ar(w,w)+=pEqn.internalCoeffs()[I][J];
//            		b[w]   +=pEqn.boundaryCoeffs()[I][J];
			br(w) +=pEqn.boundaryCoeffs()[I][J];
            	    }
                
                }

                arma::mat Ar2 = Ar;
		
// 		arma::mat Eye(Ar.n_rows,Ar.n_cols);
// 		Eye.eye();
                arma::mat D(Ar.n_rows,Ar.n_cols); // = pow(Ar%Eye,-0.5); //CHIT!!
		arma::mat Asqr = pow(abs(Ar),0.5);
		for (int i=0; i<p.size(); i++) // forAll(A,i)
                {
		  for (int j=0; j<p.size(); j++)
		  {
		    if (i==j)
		    {
		      if (Asqr(i,j)==0)
		      {
			Info << "Asqr(i,j)==0 !!!" << nl << endl;
		      }
		      else
		      {
			D(i,j)= 1/Asqr(i,j);
		      }
		    }
		  }
		}
		    
// 		arma::mat Arr = D%abs(Ar);
		arma::mat Arrr = D*Ar*D;
                		
		upSum = 0;
		for (int i=0; i<p.size(); i++) // forAll(A,i)
                {
		  for (int j=0; j<p.size(); j++)
		  {
		    if (i==j)
		    {
		      upSum += Arrr(i,j);
		      Info << "Arr("<< i << "," << j << ")" << Arrr(i,j) << endl;
		      Info << "D("<< i << "," << j << ")" << D(i,j) << endl;
		    }
		    else
		    {
		      if (Arrr(i,j)>0 || Arrr(i,j)<0)
		      {
			downSum += Arrr(i,j);
		        Info << "downSum = " << downSum << nl << endl;
		      }
		    }
		  }
		}
		
		Info << "=== Ar(i,j) ===" << nl << endl;
		for (int i=0; i<p.size(); i++) // forAll(A,i)
                {
		  for (int j=0; j<p.size(); j++)
		  {
		    Info << Ar(i,j) << " ";
		  }
		  Info << nl << endl;
		  
		}
// 
// 		Info << "=== D(i,j) ===" << nl << endl;
// 		for (int i=0; i<p.size(); i++) // forAll(A,i)
//                 {
// 		  for (int j=0; j<p.size(); j++)
// 		  {
// 		    Info << D(i,j) << " ";
// 		  }
// 		  Info << nl << endl;
// 		  
// 		}
// 
// 		Info << "=== Ar(i,j) ===" << nl << endl;
// 		for (int i=0; i<p.size(); i++) // forAll(A,i)
//                 {
// 		  for (int j=0; j<p.size(); j++)
// 		  {
// 		    Info << Ar(i,j) << " ";
// 		  }
// 		  Info << nl << endl;
// 		  
// 		}
// 		
// 		Info << "=== Arrr(i,j) ===" << nl << endl;
// 		for (int i=0; i<p.size(); i++) // forAll(A,i)
//                 {
// 		  for (int j=0; j<p.size(); j++)
// 		  {
// 		    Info << Arrr(i,j) << " ";
// 		  }
// 		  Info << nl << endl;
// 		  
// 		}
		    
		Info << "Ar(0,1)" << Ar(0,1) << nl << endl;
		Info << "Ar(1,0)" << Ar(1,0) << nl << endl;
		Info << "Ar(1,1)" << Ar(1,1) << nl << endl;
		Info << "D(1,1)" << D(1,1) << nl << endl;

		Info << "Arrr(0,1)" << Arrr(0,1) << nl << endl;
		Info << "Arrrr(1,0)" << Arrr(1,0) << nl << endl;
		Info << "Arrr(1,1)" << Arrr(1,1) << nl << endl;
		Info << "Arrr(" << p.size() << "," << p.size() << ")" << Arrr(p.size()-1,p.size()-1) << nl << endl;
                scalar x = arma::det(Ar);
		Info << "det(Ar)" <<nl<<x<<nl<<endl;
		arma::Col<std::complex<double> > eig = arma::eig_gen(Ar);
		scalar minEig = min(eig).real();
		scalar maxEig = max(eig).real();
		if (min(eig).imag()==0 && max(eig).imag()==0)
		Info << "Imaginary parts of minEig and maxEig are zeros" << nl << endl;
		
		Info << "minEig" << nl<< minEig <<nl<<endl;
		Info << "maxEig" << nl<< maxEig <<nl<<endl;
		Info << "Cond^-1" << nl<< minEig/maxEig <<nl<<endl;
		F = upSum*upSum/downSum;
		Info << "upSum=" << upSum << nl << endl;
		Info << "downSum=" << downSum << nl << endl;
		Info << "F=" << F << nl << endl;
		Info << "Ar(0,0)" << Ar(0,0) << nl << endl;

//
		
//----------------------------------------------------------------- 

//         if (nonOrth == nNonOrthCorr)
//         {
//             phi -= pEqn.flux();
//         }
    }

//     Info<< "continuity error = "
//         << mag(fvc::div(phi))().weightedAverage(mesh.V()).value()
//         << endl;

//     U = fvc::reconstruct(phi);
//     U.correctBoundaryConditions();

    Info<< "Interpolated U error = "
//         << (sqrt(sum(sqr((fvc::interpolate(U) & mesh.Sf()) - phi)))
//           /sum(mesh.magSf())).value()
        << endl;

    // Force the write
//     U.write();
//     phi.write();

    if (args.optionFound("writep"))
    {
//         p.write();
    }

    runTime.functionObjects().end();

    Info<< "ExecutionTime = " << runTime.elapsedCpuTime() << " s"
        << "  ClockTime = " << runTime.elapsedClockTime() << " s"
        << nl << endl;

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
