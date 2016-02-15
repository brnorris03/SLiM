//
//  slim_global.cpp
//  SLiM
//
//  Created by Ben Haller on 1/4/15.
//  Copyright (c) 2014 Philipp Messer.  All rights reserved.
//	A product of the Messer Lab, http://messerlab.org/software/
//

//	This file is part of SLiM.
//
//	SLiM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//
//	SLiM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License along with SLiM.  If not, see <http://www.gnu.org/licenses/>.


#include "slim_global.h"
#include "mutation.h"


void SLiM_WarmUp(void)
{
	static bool been_here = false;
	
	if (!been_here)
	{
		been_here = true;
		
		// Set up our shared pool for Mutation objects
		gSLiM_Mutation_Pool = new EidosObjectPool(sizeof(Mutation));
		
		// Register global strings and IDs for SLiM; this is in addition to the globals set up by Eidos
		SLiM_RegisterGlobalStringsAndIDs();
	}
}


// a stringstream for SLiM output; see the header for details
std::ostringstream gSLiMOut;


// Functions for casting from Eidos ints (int64_t) to SLiM int types safely
void SLiMRaiseGenerationRangeError(int64_t p_long_value)
{
	EIDOS_TERMINATION << "ERROR (SLiMRaiseGenerationRangeError): value " << p_long_value << " for a generation index or duration is out of range." << eidos_terminate();
}

void SLiMRaisePositionRangeError(int64_t p_long_value)
{
	EIDOS_TERMINATION << "ERROR (SLiMRaisePositionRangeError): value " << p_long_value << " for a chromosome position or length is out of range." << eidos_terminate();
}

void SLiMRaiseObjectidRangeError(int64_t p_long_value)
{
	EIDOS_TERMINATION << "ERROR (SLiMRaiseObjectidRangeError): value " << p_long_value << " for a SLiM object identifier value is out of range." << eidos_terminate();
}

void SLiMRaisePopsizeRangeError(int64_t p_long_value)
{
	EIDOS_TERMINATION << "ERROR (SLiMRaisePopsizeRangeError): value " << p_long_value << " for a subpopulation size, individual index, or genome index is out of range." << eidos_terminate();
}

void SLiMRaiseUsertagRangeError(int64_t p_long_value)
{
	EIDOS_TERMINATION << "ERROR (SLiMRaiseUsertagRangeError): value " << p_long_value << " for a user-supplied tag is out of range." << eidos_terminate();
}


// stream output for enumerations
std::ostream& operator<<(std::ostream& p_out, GenomeType p_genome_type)
{
	switch (p_genome_type)
	{
		case GenomeType::kAutosome:		p_out << gStr_A; break;
		case GenomeType::kXChromosome:	p_out << gStr_X; break;	// SEX ONLY
		case GenomeType::kYChromosome:	p_out << gStr_Y; break;	// SEX ONLY
	}
	
	return p_out;
}

std::ostream& operator<<(std::ostream& p_out, IndividualSex p_sex)
{
	switch (p_sex)
	{
		case IndividualSex::kUnspecified:		p_out << "*"; break;
		case IndividualSex::kHermaphrodite:		p_out << "H"; break;
		case IndividualSex::kFemale:			p_out << "F"; break;	// SEX ONLY
		case IndividualSex::kMale:				p_out << "M"; break;	// SEX ONLY
	}
	
	return p_out;
}

// initialize...() functions defined by SLiMSim
const std::string gStr_initializeGenomicElement = "initializeGenomicElement";
const std::string gStr_initializeGenomicElementType = "initializeGenomicElementType";
const std::string gStr_initializeMutationType = "initializeMutationType";
const std::string gStr_initializeGeneConversion = "initializeGeneConversion";
const std::string gStr_initializeMutationRate = "initializeMutationRate";
const std::string gStr_initializeRecombinationRate = "initializeRecombinationRate";
const std::string gStr_initializeSex = "initializeSex";

// mostly property names
const std::string gStr_genomicElements = "genomicElements";
const std::string gStr_lastPosition = "lastPosition";
const std::string gStr_overallRecombinationRate = "overallRecombinationRate";
const std::string gStr_recombinationEndPositions = "recombinationEndPositions";
const std::string gStr_recombinationRates = "recombinationRates";
const std::string gStr_geneConversionFraction = "geneConversionFraction";
const std::string gStr_geneConversionMeanLength = "geneConversionMeanLength";
const std::string gStr_overallMutationRate = "overallMutationRate";
const std::string gStr_genomeType = "genomeType";
const std::string gStr_isNullGenome = "isNullGenome";
const std::string gStr_mutations = "mutations";
const std::string gStr_genomicElementType = "genomicElementType";
const std::string gStr_startPosition = "startPosition";
const std::string gStr_endPosition = "endPosition";
const std::string gStr_id = "id";
const std::string gStr_mutationTypes = "mutationTypes";
const std::string gStr_mutationFractions = "mutationFractions";
const std::string gStr_mutationType = "mutationType";
const std::string gStr_originGeneration = "originGeneration";
const std::string gStr_position = "position";
const std::string gStr_selectionCoeff = "selectionCoeff";
const std::string gStr_subpopID = "subpopID";
const std::string gStr_convertToSubstitution = "convertToSubstitution";
const std::string gStr_distributionType = "distributionType";
const std::string gStr_distributionParams = "distributionParams";
const std::string gStr_dominanceCoeff = "dominanceCoeff";
const std::string gStr_start = "start";
const std::string gStr_end = "end";
const std::string gStr_type = "type";
const std::string gStr_source = "source";
const std::string gStr_active = "active";
const std::string gStr_chromosome = "chromosome";
const std::string gStr_chromosomeType = "chromosomeType";
const std::string gStr_genomicElementTypes = "genomicElementTypes";
const std::string gStr_scriptBlocks = "scriptBlocks";
const std::string gStr_sexEnabled = "sexEnabled";
const std::string gStr_subpopulations = "subpopulations";
const std::string gStr_substitutions = "substitutions";
const std::string gStr_dominanceCoeffX = "dominanceCoeffX";
const std::string gStr_generation = "generation";
const std::string gStr_tag = "tag";
const std::string gStr_firstMaleIndex = "firstMaleIndex";
const std::string gStr_genomes = "genomes";
const std::string gStr_immigrantSubpopIDs = "immigrantSubpopIDs";
const std::string gStr_immigrantSubpopFractions = "immigrantSubpopFractions";
const std::string gStr_selfingRate = "selfingRate";
const std::string gStr_cloningRate = "cloningRate";
const std::string gStr_sexRatio = "sexRatio";
const std::string gStr_individualCount = "individualCount";
const std::string gStr_fixationGeneration = "fixationGeneration";

// mostly method names
const std::string gStr_setRecombinationRate = "setRecombinationRate";
const std::string gStr_addMutations = "addMutations";
const std::string gStr_addNewDrawnMutation = "addNewDrawnMutation";
const std::string gStr_addNewMutation = "addNewMutation";
const std::string gStr_containsMutations = "containsMutations";
const std::string gStr_countOfMutationsOfType = "countOfMutationsOfType";
const std::string gStr_mutationsOfType = "mutationsOfType";
const std::string gStr_removeMutations = "removeMutations";
const std::string gStr_setGenomicElementType = "setGenomicElementType";
const std::string gStr_setMutationFractions = "setMutationFractions";
const std::string gStr_setSelectionCoeff = "setSelectionCoeff";
const std::string gStr_setDistribution = "setDistribution";
const std::string gStr_addSubpop = "addSubpop";
const std::string gStr_addSubpopSplit = "addSubpopSplit";
const std::string gStr_deregisterScriptBlock = "deregisterScriptBlock";
const std::string gStr_mutationFrequencies = "mutationFrequencies";
//const std::string gStr_mutationsOfType = "mutationsOfType";
//const std::string gStr_countOfMutationsOfType = "countOfMutationsOfType";
const std::string gStr_outputFixedMutations = "outputFixedMutations";
const std::string gStr_outputFull = "outputFull";
const std::string gStr_outputMutations = "outputMutations";
const std::string gStr_readFromPopulationFile = "readFromPopulationFile";
const std::string gStr_recalculateFitness = "recalculateFitness";
const std::string gStr_registerEarlyEvent = "registerEarlyEvent";
const std::string gStr_registerLateEvent = "registerLateEvent";
const std::string gStr_registerFitnessCallback = "registerFitnessCallback";
const std::string gStr_registerMateChoiceCallback = "registerMateChoiceCallback";
const std::string gStr_registerModifyChildCallback = "registerModifyChildCallback";
const std::string gStr_simulationFinished = "simulationFinished";
const std::string gStr_setMigrationRates = "setMigrationRates";
const std::string gStr_setCloningRate = "setCloningRate";
const std::string gStr_setSelfingRate = "setSelfingRate";
const std::string gStr_setSexRatio = "setSexRatio";
const std::string gStr_setSubpopulationSize = "setSubpopulationSize";
const std::string gStr_cachedFitness = "cachedFitness";
const std::string gStr_outputMSSample = "outputMSSample";
const std::string gStr_outputSample = "outputSample";

// mostly SLiM variable names used in callbacks and such
const std::string gStr_sim = "sim";
const std::string gStr_self = "self";
const std::string gStr_genome1 = "genome1";
const std::string gStr_genome2 = "genome2";
const std::string gStr_subpop = "subpop";
const std::string gStr_sourceSubpop = "sourceSubpop";
const std::string gStr_weights = "weights";
const std::string gStr_childGenome1 = "childGenome1";
const std::string gStr_childGenome2 = "childGenome2";
const std::string gStr_childIsFemale = "childIsFemale";
const std::string gStr_parent1Genome1 = "parent1Genome1";
const std::string gStr_parent1Genome2 = "parent1Genome2";
const std::string gStr_isCloning = "isCloning";
const std::string gStr_isSelfing = "isSelfing";
const std::string gStr_parent2Genome1 = "parent2Genome1";
const std::string gStr_parent2Genome2 = "parent2Genome2";
const std::string gStr_mut = "mut";
const std::string gStr_relFitness = "relFitness";
const std::string gStr_homozygous = "homozygous";

// mostly SLiM element types
const std::string gStr_Chromosome = "Chromosome";
const std::string gStr_Genome = "Genome";
const std::string gStr_GenomicElement = "GenomicElement";
const std::string gStr_GenomicElementType = "GenomicElementType";
const std::string gStr_Mutation = "Mutation";
const std::string gStr_MutationType = "MutationType";
const std::string gStr_SLiMEidosBlock = "SLiMEidosBlock";
const std::string gStr_SLiMSim = "SLiMSim";
const std::string gStr_Subpopulation = "Subpopulation";
const std::string gStr_Substitution = "Substitution";

// mostly other fixed strings
const std::string gStr_A = "A";
const std::string gStr_X = "X";
const std::string gStr_Y = "Y";
const std::string gStr_f = "f";
const std::string gStr_g = "g";
const std::string gStr_e = "e";
const std::string gStr_n = "n";
const std::string gStr_w = "w";
const std::string gStr_early = "early";
const std::string gStr_late = "late";
const std::string gStr_initialize = "initialize";
const std::string gStr_fitness = "fitness";
const std::string gStr_mateChoice = "mateChoice";
const std::string gStr_modifyChild = "modifyChild";


void SLiM_RegisterGlobalStringsAndIDs(void)
{
	static bool been_here = false;
	
	if (!been_here)
	{
		been_here = true;
		
		gEidosContextVersion = "SLiM version 2.0a3";
		gEidosContextLicense = "SLiM is free software: you can redistribute it and/or\nmodify it under the terms of the GNU General Public\nLicense as published by the Free Software Foundation,\neither version 3 of the License, or (at your option)\nany later version.\n\nSLiM is distributed in the hope that it will be\nuseful, but WITHOUT ANY WARRANTY; without even the\nimplied warranty of MERCHANTABILITY or FITNESS FOR\nA PARTICULAR PURPOSE.  See the GNU General Public\nLicense for more details.\n\nYou should have received a copy of the GNU General\nPublic License along with SLiM.  If not, see\n<http://www.gnu.org/licenses/>.";
		
		Eidos_RegisterStringForGlobalID(gStr_initializeGenomicElement, gID_initializeGenomicElement);
		Eidos_RegisterStringForGlobalID(gStr_initializeGenomicElementType, gID_initializeGenomicElementType);
		Eidos_RegisterStringForGlobalID(gStr_initializeMutationType, gID_initializeMutationType);
		Eidos_RegisterStringForGlobalID(gStr_initializeGeneConversion, gID_initializeGeneConversion);
		Eidos_RegisterStringForGlobalID(gStr_initializeMutationRate, gID_initializeMutationRate);
		Eidos_RegisterStringForGlobalID(gStr_initializeRecombinationRate, gID_initializeRecombinationRate);
		Eidos_RegisterStringForGlobalID(gStr_initializeSex, gID_initializeSex);
		
		Eidos_RegisterStringForGlobalID(gStr_genomicElements, gID_genomicElements);
		Eidos_RegisterStringForGlobalID(gStr_lastPosition, gID_lastPosition);
		Eidos_RegisterStringForGlobalID(gStr_overallRecombinationRate, gID_overallRecombinationRate);
		Eidos_RegisterStringForGlobalID(gStr_recombinationEndPositions, gID_recombinationEndPositions);
		Eidos_RegisterStringForGlobalID(gStr_recombinationRates, gID_recombinationRates);
		Eidos_RegisterStringForGlobalID(gStr_geneConversionFraction, gID_geneConversionFraction);
		Eidos_RegisterStringForGlobalID(gStr_geneConversionMeanLength, gID_geneConversionMeanLength);
		Eidos_RegisterStringForGlobalID(gStr_overallMutationRate, gID_overallMutationRate);
		Eidos_RegisterStringForGlobalID(gStr_genomeType, gID_genomeType);
		Eidos_RegisterStringForGlobalID(gStr_isNullGenome, gID_isNullGenome);
		Eidos_RegisterStringForGlobalID(gStr_mutations, gID_mutations);
		Eidos_RegisterStringForGlobalID(gStr_genomicElementType, gID_genomicElementType);
		Eidos_RegisterStringForGlobalID(gStr_startPosition, gID_startPosition);
		Eidos_RegisterStringForGlobalID(gStr_endPosition, gID_endPosition);
		Eidos_RegisterStringForGlobalID(gStr_id, gID_id);
		Eidos_RegisterStringForGlobalID(gStr_mutationTypes, gID_mutationTypes);
		Eidos_RegisterStringForGlobalID(gStr_mutationFractions, gID_mutationFractions);
		Eidos_RegisterStringForGlobalID(gStr_mutationType, gID_mutationType);
		Eidos_RegisterStringForGlobalID(gStr_originGeneration, gID_originGeneration);
		Eidos_RegisterStringForGlobalID(gStr_position, gID_position);
		Eidos_RegisterStringForGlobalID(gStr_selectionCoeff, gID_selectionCoeff);
		Eidos_RegisterStringForGlobalID(gStr_subpopID, gID_subpopID);
		Eidos_RegisterStringForGlobalID(gStr_convertToSubstitution, gID_convertToSubstitution);
		Eidos_RegisterStringForGlobalID(gStr_distributionType, gID_distributionType);
		Eidos_RegisterStringForGlobalID(gStr_distributionParams, gID_distributionParams);
		Eidos_RegisterStringForGlobalID(gStr_dominanceCoeff, gID_dominanceCoeff);
		Eidos_RegisterStringForGlobalID(gStr_start, gID_start);
		Eidos_RegisterStringForGlobalID(gStr_end, gID_end);
		Eidos_RegisterStringForGlobalID(gStr_type, gID_type);
		Eidos_RegisterStringForGlobalID(gStr_source, gID_source);
		Eidos_RegisterStringForGlobalID(gStr_active, gID_active);
		Eidos_RegisterStringForGlobalID(gStr_chromosome, gID_chromosome);
		Eidos_RegisterStringForGlobalID(gStr_chromosomeType, gID_chromosomeType);
		Eidos_RegisterStringForGlobalID(gStr_genomicElementTypes, gID_genomicElementTypes);
		Eidos_RegisterStringForGlobalID(gStr_scriptBlocks, gID_scriptBlocks);
		Eidos_RegisterStringForGlobalID(gStr_sexEnabled, gID_sexEnabled);
		Eidos_RegisterStringForGlobalID(gStr_subpopulations, gID_subpopulations);
		Eidos_RegisterStringForGlobalID(gStr_substitutions, gID_substitutions);
		Eidos_RegisterStringForGlobalID(gStr_dominanceCoeffX, gID_dominanceCoeffX);
		Eidos_RegisterStringForGlobalID(gStr_generation, gID_generation);
		Eidos_RegisterStringForGlobalID(gStr_tag, gID_tag);
		Eidos_RegisterStringForGlobalID(gStr_firstMaleIndex, gID_firstMaleIndex);
		Eidos_RegisterStringForGlobalID(gStr_genomes, gID_genomes);
		Eidos_RegisterStringForGlobalID(gStr_immigrantSubpopIDs, gID_immigrantSubpopIDs);
		Eidos_RegisterStringForGlobalID(gStr_immigrantSubpopFractions, gID_immigrantSubpopFractions);
		Eidos_RegisterStringForGlobalID(gStr_selfingRate, gID_selfingRate);
		Eidos_RegisterStringForGlobalID(gStr_cloningRate, gID_cloningRate);
		Eidos_RegisterStringForGlobalID(gStr_sexRatio, gID_sexRatio);
		Eidos_RegisterStringForGlobalID(gStr_individualCount, gID_individualCount);
		Eidos_RegisterStringForGlobalID(gStr_fixationGeneration, gID_fixationGeneration);
		
		Eidos_RegisterStringForGlobalID(gStr_setRecombinationRate, gID_setRecombinationRate);
		Eidos_RegisterStringForGlobalID(gStr_addMutations, gID_addMutations);
		Eidos_RegisterStringForGlobalID(gStr_addNewDrawnMutation, gID_addNewDrawnMutation);
		Eidos_RegisterStringForGlobalID(gStr_addNewMutation, gID_addNewMutation);
		Eidos_RegisterStringForGlobalID(gStr_countOfMutationsOfType, gID_countOfMutationsOfType);
		Eidos_RegisterStringForGlobalID(gStr_containsMutations, gID_containsMutations);
		Eidos_RegisterStringForGlobalID(gStr_mutationsOfType, gID_mutationsOfType);
		Eidos_RegisterStringForGlobalID(gStr_removeMutations, gID_removeMutations);
		Eidos_RegisterStringForGlobalID(gStr_setGenomicElementType, gID_setGenomicElementType);
		Eidos_RegisterStringForGlobalID(gStr_setMutationFractions, gID_setMutationFractions);
		Eidos_RegisterStringForGlobalID(gStr_setSelectionCoeff, gID_setSelectionCoeff);
		Eidos_RegisterStringForGlobalID(gStr_setDistribution, gID_setDistribution);
		Eidos_RegisterStringForGlobalID(gStr_addSubpop, gID_addSubpop);
		Eidos_RegisterStringForGlobalID(gStr_addSubpopSplit, gID_addSubpopSplit);
		Eidos_RegisterStringForGlobalID(gStr_deregisterScriptBlock, gID_deregisterScriptBlock);
		Eidos_RegisterStringForGlobalID(gStr_mutationFrequencies, gID_mutationFrequencies);
		//Eidos_RegisterStringForGlobalID(gStr_mutationsOfType, gID_mutationsOfType);
		//Eidos_RegisterStringForGlobalID(gStr_countOfMutationsOfType, gID_countOfMutationsOfType);
		Eidos_RegisterStringForGlobalID(gStr_outputFixedMutations, gID_outputFixedMutations);
		Eidos_RegisterStringForGlobalID(gStr_outputFull, gID_outputFull);
		Eidos_RegisterStringForGlobalID(gStr_outputMutations, gID_outputMutations);
		Eidos_RegisterStringForGlobalID(gStr_readFromPopulationFile, gID_readFromPopulationFile);
		Eidos_RegisterStringForGlobalID(gStr_recalculateFitness, gID_recalculateFitness);
		Eidos_RegisterStringForGlobalID(gStr_registerEarlyEvent, gID_registerEarlyEvent);
		Eidos_RegisterStringForGlobalID(gStr_registerLateEvent, gID_registerLateEvent);
		Eidos_RegisterStringForGlobalID(gStr_registerFitnessCallback, gID_registerFitnessCallback);
		Eidos_RegisterStringForGlobalID(gStr_registerMateChoiceCallback, gID_registerMateChoiceCallback);
		Eidos_RegisterStringForGlobalID(gStr_registerModifyChildCallback, gID_registerModifyChildCallback);
		Eidos_RegisterStringForGlobalID(gStr_simulationFinished, gID_simulationFinished);
		Eidos_RegisterStringForGlobalID(gStr_setMigrationRates, gID_setMigrationRates);
		Eidos_RegisterStringForGlobalID(gStr_setCloningRate, gID_setCloningRate);
		Eidos_RegisterStringForGlobalID(gStr_setSelfingRate, gID_setSelfingRate);
		Eidos_RegisterStringForGlobalID(gStr_setSexRatio, gID_setSexRatio);
		Eidos_RegisterStringForGlobalID(gStr_setSubpopulationSize, gID_setSubpopulationSize);
		Eidos_RegisterStringForGlobalID(gStr_cachedFitness, gID_cachedFitness);
		Eidos_RegisterStringForGlobalID(gStr_outputMSSample, gID_outputMSSample);
		Eidos_RegisterStringForGlobalID(gStr_outputSample, gID_outputSample);
		
		Eidos_RegisterStringForGlobalID(gStr_sim, gID_sim);
		Eidos_RegisterStringForGlobalID(gStr_self, gID_self);
		Eidos_RegisterStringForGlobalID(gStr_genome1, gID_genome1);
		Eidos_RegisterStringForGlobalID(gStr_genome2, gID_genome2);
		Eidos_RegisterStringForGlobalID(gStr_subpop, gID_subpop);
		Eidos_RegisterStringForGlobalID(gStr_sourceSubpop, gID_sourceSubpop);
		Eidos_RegisterStringForGlobalID(gStr_weights, gID_weights);
		Eidos_RegisterStringForGlobalID(gStr_childGenome1, gID_childGenome1);
		Eidos_RegisterStringForGlobalID(gStr_childGenome2, gID_childGenome2);
		Eidos_RegisterStringForGlobalID(gStr_childIsFemale, gID_childIsFemale);
		Eidos_RegisterStringForGlobalID(gStr_parent1Genome1, gID_parent1Genome1);
		Eidos_RegisterStringForGlobalID(gStr_parent1Genome2, gID_parent1Genome2);
		Eidos_RegisterStringForGlobalID(gStr_isCloning, gID_isCloning);
		Eidos_RegisterStringForGlobalID(gStr_isSelfing, gID_isSelfing);
		Eidos_RegisterStringForGlobalID(gStr_parent2Genome1, gID_parent2Genome1);
		Eidos_RegisterStringForGlobalID(gStr_parent2Genome2, gID_parent2Genome2);
		Eidos_RegisterStringForGlobalID(gStr_mut, gID_mut);
		Eidos_RegisterStringForGlobalID(gStr_relFitness, gID_relFitness);
		Eidos_RegisterStringForGlobalID(gStr_homozygous, gID_homozygous);
		
		Eidos_RegisterStringForGlobalID(gStr_Chromosome, gID_Chromosome);
		Eidos_RegisterStringForGlobalID(gStr_Genome, gID_Genome);
		Eidos_RegisterStringForGlobalID(gStr_GenomicElement, gID_GenomicElement);
		Eidos_RegisterStringForGlobalID(gStr_GenomicElementType, gID_GenomicElementType);
		Eidos_RegisterStringForGlobalID(gStr_Mutation, gID_Mutation);
		Eidos_RegisterStringForGlobalID(gStr_MutationType, gID_MutationType);
		Eidos_RegisterStringForGlobalID(gStr_SLiMEidosBlock, gID_SLiMEidosBlock);
		Eidos_RegisterStringForGlobalID(gStr_SLiMSim, gID_SLiMSim);
		Eidos_RegisterStringForGlobalID(gStr_Subpopulation, gID_Subpopulation);
		Eidos_RegisterStringForGlobalID(gStr_Substitution, gID_Substitution);
		
		Eidos_RegisterStringForGlobalID(gStr_A, gID_A);
		Eidos_RegisterStringForGlobalID(gStr_X, gID_X);
		Eidos_RegisterStringForGlobalID(gStr_Y, gID_Y);
		Eidos_RegisterStringForGlobalID(gStr_f, gID_f);
		Eidos_RegisterStringForGlobalID(gStr_g, gID_g);
		Eidos_RegisterStringForGlobalID(gStr_e, gID_e);
		Eidos_RegisterStringForGlobalID(gStr_n, gID_n);
		Eidos_RegisterStringForGlobalID(gStr_w, gID_w);
		Eidos_RegisterStringForGlobalID(gStr_early, gID_early);
		Eidos_RegisterStringForGlobalID(gStr_late, gID_late);
		Eidos_RegisterStringForGlobalID(gStr_initialize, gID_initialize);
		Eidos_RegisterStringForGlobalID(gStr_fitness, gID_fitness);
		Eidos_RegisterStringForGlobalID(gStr_mateChoice, gID_mateChoice);
		Eidos_RegisterStringForGlobalID(gStr_modifyChild, gID_modifyChild);
	}
}




























































