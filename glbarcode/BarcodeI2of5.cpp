/*  BarcodeCode39.cpp
 *
 *  Copyright (C) 2013  Jim Evins <evins@snaught.com>
 *
 *  This file is part of glbarcode++.
 *
 *  glbarcode++ is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  glbarcode++ is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with glbarcode++.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "BarcodeI2of5.h"

#include "Constants.h"

#include <cctype>
#include <algorithm>


using namespace glbarcode::Constants;


namespace
{
	/* Code 39 alphabet. Position indicates value. */
	const std::string alphabet = "0123456789";

	/* Code 39 symbols. Position must match position in alphabet. */
	const std::string symbols[] = {
		/*        BsBsBsBsB */
		/* 0 */  "nnwwn",
		/* 1 */  "wnnnw",
		/* 2 */  "nwnnw",
		/* 3 */  "wwnnn",
		/* 4 */  "nnwnw",
		/* 5 */  "wnwnn",
		/* 6 */  "nwwnn",
		/* 7 */  "nnnww",
		/* 8 */  "wnnwn",
		/* 9 */  "nwnwn",
	};

	const std::string frameSymbol = "NnNn";
	const std::string frameEndSymbol = "WnN";

	/* Vectorization constants */
	const double MIN_X       = ( 0.0075 *  PTS_PER_INCH );
	const double N           = 2.5;
	//const double MIN_I       = MIN_X;
	const double MIN_HEIGHT  = ( 0.19685 *  PTS_PER_INCH );
	const double MIN_QUIET   = ( 10 * MIN_X );

	const double MIN_TEXT_AREA_HEIGHT = 12.0;
	const double MIN_TEXT_SIZE        = 8.0;

}


namespace glbarcode
{

	/*
	 * Static Code39 barcode creation method
	 */
	Barcode* BarcodeI2of5::create( )
	{
		return new BarcodeI2of5();
	}


	/*
	 * Code39 data validation, implements Barcode1dBase::validate()
	 */
	bool BarcodeI2of5::validate( const std::string& rawData )
	{
		if(rawData.length() % 2)
		{
			return false;
		}

		for (char r : rawData)
		{
			char c = toupper( r );

			if ( alphabet.find(c) == std::string::npos )
			{
				return false;
			}
		}

		return true;
	}


	/*
	 * Code39 data encoding, implements Barcode1dBase::encode()
	 */
	std::string BarcodeI2of5::encode( const std::string& cookedData )
	{
		std::string code;

		/* Left frame symbol */
		code += frameSymbol;

		//int sum = 0;
		for (auto i = 0u; i < cookedData.length(); i += 2)
		{
			char c1 = cookedData[i];
			char c2 = cookedData[i + 1];

			auto sym1 = symbols[alphabet.find( toupper( c1 ) )];
			auto sym2 = symbols[alphabet.find( toupper( c2 ) )];

			for(auto j = 0; j < 10; j++)
			{
				if((j % 2) == 0)
				{
					code += toupper(sym1[j / 2]);
				}
				else
				{
					code += tolower( sym2[j / 2] );
				}
			}

			//sum += int(cValue);
		}

		//if ( checksum() )
		//{
		//	code += symbols[sum % 43];
		//	code += "i";
		//}

		/* Right frame bar */
		code += frameEndSymbol;

		return code;
	}


	/*
	 * Code39 prepare text for display, implements Barcode1dBase::prepareText()
	 */
	std::string BarcodeI2of5::prepareText( const std::string& rawData )
	{
		std::string displayText;

		for (char c : rawData)
		{
			displayText += toupper( c );
		}

		return displayText;
	}


	/*
	 * Code39 vectorization, implements Barcode1dBase::vectorize()
	 */
	void BarcodeI2of5::vectorize( const std::string& codedData,
	                               const std::string& displayText,
	                               const std::string& cookedData,
	                               double&            w,
	                               double&            h )
	{

		/* determine width and establish horizontal scale, based on original cooked data */
		auto dataSize = double( cookedData.size() ) / 2.0;
		double minL;
		if ( !checksum() )
		{
			//minL = (dataSize + 2)*(3*N + 6)*MIN_X;
			minL = 2.0 * dataSize * ( 2 * N + 3) * MIN_X;
		}
		else
		{
			minL = (dataSize + 3)*(3*N + 6)*MIN_X;
			//minL = dataSize * ( 2 * N + 3) * MIN_X;
		}
        
		double scale;
		if ( w == 0 )
		{
			scale = 1.0;
		}
		else
		{
			scale = w / (minL + 2*MIN_QUIET);

			if ( scale < 1.0 )
			{
				scale = 1.0;
			}
		}
		double width = minL * scale;

		/* determine text parameters */
		double hTextArea = scale * MIN_TEXT_AREA_HEIGHT;
		double textSize   = scale * MIN_TEXT_SIZE;

		/* determine height of barcode */
		double height = showText() ? h - hTextArea : h;
		height = std::max( height, std::max( 0.15*width, MIN_HEIGHT ) );

		/* determine horizontal quiet zone */
		double xQuiet = std::max( (10 * scale * MIN_X), MIN_QUIET );

		/* Now traverse the code string and draw each bar */
		double x1 = xQuiet;
		for (char c : codedData)
		{
			double lwidth;
				
			switch ( c )
			{
#if 0
			case 'i':
				/* Inter-character gap */
				x1 += scale * MIN_I;
				break;
#endif
			case 'N':
				/* Narrow bar */
				lwidth = scale*MIN_X;
				addLine( x1, 0.0, lwidth, height );
				x1 += scale * MIN_X;
				break;

			case 'W':
				/* Wide bar */
				lwidth = scale*N*MIN_X;
				addLine( x1, 0.0, lwidth, height );
				x1 += scale * N * MIN_X;
				break;

			case 'n':
				/* Narrow space */
				x1 += scale * MIN_X;
				break;

			case 'w':
				/* Wide space */
				x1 += scale * N * MIN_X;
				break;

			default:
				// NOT REACHED
				break;
			}
		}

		if ( showText() )
		{
			std::string starredText = displayText;
			addText( xQuiet + width/2, height + (hTextArea+0.7*textSize)/2, textSize, starredText );
		}

		/* Overwrite requested size with actual size. */
		w = width + 2*xQuiet;
		h = showText() ? height + hTextArea : height;

	}

}
