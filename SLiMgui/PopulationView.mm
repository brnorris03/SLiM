//
//  PopulationView.m
//  SLiM
//
//  Created by Ben Haller on 1/21/15.
//  Copyright (c) 2015 Messer Lab, http://messerlab.org/software/. All rights reserved.
//

#import "PopulationView.h"
#import "SLiMWindowController.h"
#import "CocoaExtra.h"

#import <OpenGL/OpenGL.h>
#include <OpenGL/glu.h>


@implementation PopulationView

- (void)dealloc
{
	//NSLog(@"[PopulationView dealloc]");
	
	[super dealloc];
}

// Called after being summoned from a NIB/XIB.
- (void)prepareOpenGL
{
}

// Adjust the OpenGL viewport in response to window resize
- (void)reshape
{
}

- (void)drawViewFrameInBounds:(NSRect)bounds
{
	int ox = (int)bounds.origin.x, oy = (int)bounds.origin.y;
	
	glColor3f(0.77f, 0.77f, 0.77f);
	glRecti(ox, oy, ox + 1, oy + (int)bounds.size.height);
	glRecti(ox + 1, oy, ox + (int)bounds.size.width - 1, oy + 1);
	glRecti(ox + (int)bounds.size.width - 1, oy, ox + (int)bounds.size.width, oy + (int)bounds.size.height);
	glRecti(ox + 1, oy + (int)bounds.size.height - 1, ox + (int)bounds.size.width - 1, oy + (int)bounds.size.height);
}

#define SLIM_MAX_GL_RECTS		2000						// 2000 rects
#define SLIM_MAX_VERTICES		(SLIM_MAX_GL_RECTS * 4)		// 4 vertices each

- (void)drawIndividualsFromSubpopulation:(Subpopulation *)subpop inArea:(NSRect)bounds
{
	SLiMWindowController *controller = [[self window] windowController];
	double scalingFactor = controller->fitnessColorScale;
	int subpopSize = subpop->child_subpop_size_;
	int squareSize, viewColumns = 0, viewRows = 0;
	
	// first figure out the biggest square size that will allow us to display the whole subpopulation
	for (squareSize = 20; squareSize > 1; --squareSize)
	{
		viewColumns = (int)floor((bounds.size.width - 3) / squareSize);
		viewRows = (int)floor((bounds.size.height - 3) / squareSize);
		
		if (viewColumns * viewRows > subpopSize)
		{
			// If we have an empty row at the bottom, then break for sure; this allows us to look nice and symmetrical
			if ((subpopSize - 1) / viewColumns < viewRows - 1)
				break;
			
			// Otherwise, break only if we are getting uncomfortably small; otherwise, let's drop down one square size to allow symmetry
			if (squareSize <= 5)
				break;
		}
	}
	
	if (squareSize > 1)
	{
		int squareSpacing = 0;
		
		// Convert square area to space between squares if possible
		if (squareSize > 2)
		{
			--squareSize;
			++squareSpacing;
		}
		if (squareSize > 5)
		{
			--squareSize;
			++squareSpacing;
		}
		
		double excessSpaceX = bounds.size.width - ((squareSize + squareSpacing) * viewColumns - squareSpacing);
		double excessSpaceY = bounds.size.height - ((squareSize + squareSpacing) * viewRows - squareSpacing);
		int offsetX = (int)floor(excessSpaceX / 2.0);
		int offsetY = (int)floor(excessSpaceY / 2.0);
		
		// If we have an empty row at the bottom, then we can use the same value for offsetY as for offsetX, for symmetry
		if ((subpopSize - 1) / viewColumns < viewRows - 1)
			offsetY = offsetX;
		
		NSRect individualArea = NSMakeRect(bounds.origin.x + offsetX, bounds.origin.y + offsetY, bounds.size.width - offsetX, bounds.size.height - offsetY);
		
		static float *glArrayVertices = nil;
		static float *glArrayColors = nil;
		int individualArrayIndex, displayListIndex;
		float *vertices = NULL, *colors = NULL;
		
		// Set up the vertex and color arrays
		if (!glArrayVertices)
			glArrayVertices = (float *)malloc(SLIM_MAX_VERTICES * 2 * sizeof(float));		// 2 floats per vertex, AK_POPULATION_VIEW_GL_ARRAY_SIZE vertices
		
		if (!glArrayColors)
			glArrayColors = (float *)malloc(SLIM_MAX_VERTICES * 4 * sizeof(float));		// 4 floats per color, AK_POPULATION_VIEW_GL_ARRAY_SIZE colors
		
		// Set up to draw rects
		displayListIndex = 0;
		
		vertices = glArrayVertices;
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, glArrayVertices);
		
		colors = glArrayColors;
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(4, GL_FLOAT, 0, glArrayColors);
		
		for (individualArrayIndex = 0; individualArrayIndex < subpopSize; ++individualArrayIndex)
		{
			//AKIndividual *individual = individuals[individualArrayIndex];
			
			// Figure out the rect to draw in; note we now use individualArrayIndex here, because the hit-testing code doesn't have an easy way to calculate the displayed individual index...
			float left = (float)(individualArea.origin.x + (individualArrayIndex % viewColumns) * (squareSize + squareSpacing));
			float top = (float)(individualArea.origin.y + (individualArrayIndex / viewColumns) * (squareSize + squareSpacing));
			float right = left + squareSize;
			float bottom = top + squareSize;
			
			*(vertices++) = left;
			*(vertices++) = top;
			*(vertices++) = left;
			*(vertices++) = bottom;
			*(vertices++) = right;
			*(vertices++) = bottom;
			*(vertices++) = right;
			*(vertices++) = top;
			
			float colorRed = 0.0, colorGreen = 0.0, colorBlue = 0.0, colorAlpha = 1.0;
			
			// use individual trait values to determine color
			double fitness = subpop->FitnessOfParentWithGenomeIndices(2 * individualArrayIndex, 2 * individualArrayIndex + 1);
			
			RGBForFitness(fitness, &colorRed, &colorGreen, &colorBlue, scalingFactor);
			
			for (int j = 0; j < 4; ++j)
			{
				*(colors++) = colorRed;
				*(colors++) = colorGreen;
				*(colors++) = colorBlue;
				*(colors++) = colorAlpha;
			}
			
			displayListIndex++;
			
			// If we've filled our buffers, get ready to draw more
			if (displayListIndex == SLIM_MAX_GL_RECTS)
			{
				// Draw our arrays
				glDrawArrays(GL_QUADS, 0, 4 * displayListIndex);
				
				// And get ready to draw more
				vertices = glArrayVertices;
				colors = glArrayColors;
				displayListIndex = 0;
			}
		}
		
		// Draw any leftovers
		if (displayListIndex)
		{
			// Draw our arrays
			glDrawArrays(GL_QUADS, 0, 4 * displayListIndex);
		}
		
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
	}
	else
	{
		// This is what we do if we cannot display a subpopulation because there are too many individuals in it to display
		glColor3f(0.9f, 0.9f, 1.0f);
		
		int ox = (int)bounds.origin.x, oy = (int)bounds.origin.y;
		
		glRecti(ox + 1, oy + 1, ox + (int)bounds.size.width - 1, oy + (int)bounds.size.height - 1);
	}
}

- (void)drawRect:(NSRect)rect
{
	NSRect bounds = [self bounds];
	SLiMWindowController *controller = [[self window] windowController];
	std::vector<Subpopulation*> selectedSubpopulations = [controller selectedSubpopulations];
	int selectedSubpopCount = (int)(selectedSubpopulations.size());
	
	// Update the viewport
	glViewport(0, 0, (int)bounds.size.width, (int)bounds.size.height);
	
	// Update the projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	// FIXME this is deprecated, but I can't figure out to I'm supposed to fix it... seems like I would be forced into GLKit, which I don't want to deal with...
	gluOrtho2D(0.0, (int)bounds.size.width, (int)bounds.size.height, 0.0);	// swap bottom and top to create a flipped coordinate system
#pragma clang diagnostic pop
	glMatrixMode(GL_MODELVIEW);
	
	if (selectedSubpopCount == 0)
	{
		// clear to a shade of gray
		glColor3f(0.9f, 0.9f, 0.9f);
		glRecti(0, 0, (int)bounds.size.width, (int)bounds.size.height);
		
		// Frame our view
		[self drawViewFrameInBounds:bounds];
	}
	else if (selectedSubpopCount == 1)
	{
		Subpopulation *subpop = selectedSubpopulations[0];
		
		// Clear to white
		glColor3f(1.0, 1.0, 1.0);
		glRecti(0, 0, (int)bounds.size.width, (int)bounds.size.height);
		
		// Frame our view
		[self drawViewFrameInBounds:bounds];
		
		// Draw all the individuals
		[self drawIndividualsFromSubpopulation:subpop inArea:bounds];
	}
	else
	{
		int interBoxSpace = 5;
		int totalInterbox = interBoxSpace * (selectedSubpopCount - 1);
		double boxHeight = (bounds.size.height - totalInterbox) / (double)selectedSubpopCount;
		
		// Clear to background gray; FIXME at present this hard-codes the OS X 10.10 background color...
		glColor3f(0.93f, 0.93f, 0.93f);
		glRecti(0, 0, (int)bounds.size.width, (int)bounds.size.height);
		
		for (int subpopIndex = 0; subpopIndex < selectedSubpopCount; subpopIndex++)
		{
			double boxTop = round(bounds.origin.y + subpopIndex * (interBoxSpace + boxHeight));
			double boxBottom = round(bounds.origin.y + subpopIndex * (interBoxSpace + boxHeight) + boxHeight);
			NSRect boxBounds = NSMakeRect(bounds.origin.x, boxTop, bounds.size.width, boxBottom - boxTop);
			Subpopulation *subpop = selectedSubpopulations[subpopIndex];
			
			// Clear to white
			glColor3f(1.0, 1.0, 1.0);
			glRecti((int)boxBounds.origin.x, (int)boxBounds.origin.y, (int)(boxBounds.origin.x + boxBounds.size.width), (int)(boxBounds.origin.y + boxBounds.size.height));
			
			// Frame our view
			[self drawViewFrameInBounds:boxBounds];
			
			// Draw all the individuals
			[self drawIndividualsFromSubpopulation:subpop inArea:boxBounds];
		}
	}
	
	[[self openGLContext] flushBuffer];
}

@end
























































