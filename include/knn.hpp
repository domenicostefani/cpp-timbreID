/*

timbreID - A generic classification module.

- Originally part of timbreID (Pd library module) by William Brent
- Ported for JUCE usage by Domenico Stefani, 29th April 2020
  (domenico.stefani96@gmail.com)

Porting from timbreID.c:
 -> https://github.com/wbrent/timbreID/blob/master/src/timbreID.c

**** Original LICENCE disclaimer ahead ****

Copyright 2009 William Brent

This file is part of timbreID.

timbreID is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

timbreID is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#pragma once

#include "tIDLib.hpp"

namespace tid   /* TimbreID namespace*/
{

enum class DistanceMetric
{
    euclidean = 0,
    taxi,
    correlation
};

class KNNclassifier
{
public:

    KNNclassifier(){ this->initModule(); }

    /* -------- classification methods -------- */

    /**
     * Train the model by adding a single feature vector
     * ! NOTE: Calling this is equivalent to sending data to the first inlet of
     *         Puredata external
    */
    void trainModel(const std::vector<float>& input)
    {
        if (this->normalize)
            throw std::logic_error("Cannot add more training instances when database is normalized. deactivate normalization first.");
        else if (this->numClusters != this->numInstances)
            throw std::logic_error("Cannot add more training instances when database is clustered. uncluster first.");

        t_instanceIdx instanceIdx = this->numInstances;

        this->instances.resize(this->numInstances + 1);
        this->clusters.resize(this->numInstances + 1);

        this->instances[instanceIdx].clusterMembership = instanceIdx;
        this->instances[instanceIdx].length = input.size();

        this->instances[instanceIdx].data.resize(this->instances[instanceIdx].length);

        this->clusters[instanceIdx].numMembers = 2; // 2 because we're unclustered to start, and each instance has a cluster with itself as a member, plus UINT_MAX as the 2nd element to terminate the list

        this->clusters[instanceIdx].members.resize(this->clusters[instanceIdx].numMembers);

        // init new clusterMembers
        this->clusters[instanceIdx].members[0] = instanceIdx; // first member of the cluster is the instance index
        this->clusters[instanceIdx].members[1] = UINT_MAX;

        this->clusters[instanceIdx].votes = 0;

        this->numInstances++;
        this->numClusters++;
        this->neighborhood++;

        if (this->instances[instanceIdx].length > this->maxFeatureLength)
            this->attributeDataResize(this->instances[instanceIdx].length, true);

        if (this->instances[instanceIdx].length < this->minFeatureLength)
            this->minFeatureLength = this->instances[instanceIdx].length;

        for (t_instanceIdx i = 0; i < this->instances[instanceIdx].length; ++i)
            this->instances[instanceIdx].data[i] = (float)input[i];

        //TODO: handle outlet
        // outlefloat(this->id_outlet, instanceIdx);
    }

    /**
     * Classify a single sample (one feature vector)
     * Equivalent to sending data to the second inlet
    */
    void classifySample(const std::vector<float>& input)
    {
        float dist, bestDist, secondBestDist, confidence;
        t_instanceIdx winningID, topVoteInstances;
        unsigned int topVote;
        t_attributeIdx listLength;

        if (this->numInstances)
        {
            listLength = input.size();
            confidence = 0.0;

            if (listLength > this->maxFeatureLength)
            {
                tIDLib::logError("Input feature list longer than current max feature length of database. input ignored.");
                return;
            }

            // init votes to 0
            for (t_instanceIdx i = 0; i < this->numClusters; ++i)
                this->clusters[i].votes = 0;

            // init cluster info to instance idx
            for (t_instanceIdx i = 0; i < this->numInstances; ++i)
            {
                this->instances[i].knnInfo.idx = i;
                this->instances[i].knnInfo.cluster = i;
                this->instances[i].knnInfo.dist = FLT_MAX;
                this->instances[i].knnInfo.safeDist = FLT_MAX;
            }

            for (t_instanceIdx i = 0; i < listLength; ++i)
                this->attributeData[i].inputData = (float)input[i];

            winningID = UINT_MAX;
            bestDist = FLT_MAX;
            secondBestDist = FLT_MAX;

            for (t_instanceIdx i = 0; i < this->numInstances; ++i)
            {
                dist = this->getInputDist(i);

                // abort _id() altogether if distance measurement fails. return value of FLT_MAX indicates failure
                if (dist == FLT_MAX)
                    return;

                this->instances[i].knnInfo.dist = this->instances[i].knnInfo.safeDist = dist; // store the distance
            }

            // a reduced sort, so that the first k elements in this->instances will be the ones with lowest distances to the input feature in the list, and in order to boot. NOTE: tIDLib::sortKnnInfo() does not operate on the primary instance data, just each instance's knnInfo member.
            tIDLib::sortKnnInfo(this->kValue, this->numInstances, UINT_MAX, this->instances); // pass a prevMatch value of UINT_MAX as a flag that it's unused here

            // store instance's cluster id
            for (t_instanceIdx i = 0; i < this->kValue; ++i)
            {
                t_instanceIdx thisInstance;
                thisInstance = this->instances[i].knnInfo.idx;

                this->instances[i].knnInfo.cluster = this->instances[thisInstance].clusterMembership;
            }

            // vote
            for (t_instanceIdx i = 0; i < this->kValue; ++i)
            {
                t_instanceIdx thisCluster;
                thisCluster = this->instances[i].knnInfo.cluster;

                this->clusters[thisCluster].votes++;
            }

            // TODO: potential issue is that i'm using t_uInt for t_cluster.votes and topVote, so can no longer have -1 as the initialized value. should be that initializing to 0 is actually ok, even in the case of a tie for 0. originally this was topVote = -1;
            topVote = 0;
            for (t_instanceIdx i = 0; i < this->numClusters; ++i)
            {
                if (this->clusters[i].votes > topVote)
                {
                    topVote = this->clusters[i].votes;
                    winningID = i; // store cluster id of winner
                }
            }

            // see if there is a tie for number of votes
            topVoteInstances = 0;
            for (t_instanceIdx i = 0; i < this->numClusters; ++i)
                if (this->clusters[i].votes==topVote)
                    topVoteInstances++;

            // in case of a tie, pick the instance with the shortest distance. The knnInfo for instances will be sorted by distance, so this->instance[0].knnInfo.cluster is the cluster ID of the instance with the smallest distance
            if (topVoteInstances>1)
                winningID = this->instances[0].knnInfo.cluster;

            for (t_instanceIdx i = 0; i < this->kValue; ++i)
            {
                if (this->instances[i].knnInfo.cluster==winningID)
                {
                    bestDist = this->instances[i].knnInfo.safeDist;
                    break;
                }
            }

            // this assigns the distance belonging to the first item in knnInfo that isn't a member of the winning cluster to the variable "secondBest". i.e., the distance between the test vector and the next nearest instance that isn't a member of the winning cluster.
            for (t_instanceIdx i = 0; i < this->kValue; ++i)
            {
                if (this->instances[i].knnInfo.cluster!=winningID)
                {
                    secondBestDist = this->instances[i].knnInfo.safeDist;
                    break;
                }
            }

        /*
        // unless I'm missing something, this is faulty logic with the current setup. If all K items belong to the same cluster, then secondBestDist will still be FLT_MAX, which will make confidence below near to 1.0. That's good. Assigning it the actual second best distance from a sibling cluster member will produce a number near 0 for confidence, which is wrong.

            // if no second best assignment is made (because all K items belong to same cluster), make 2nd best the 2nd in list
            if (secondBestDist==FLT_MAX)
                secondBestDist = this->instances[1].knnInfo.safeDist;

        // if above note on faulty logic is correct, then this is not needed. confidence can just be calculated without a condition
            if (secondBestDist<=0)
                confidence = -FLT_MAX;
            else
                confidence = 1.0-(bestDist/secondBestDist);
        */

            confidence = 1.0 - (bestDist / secondBestDist);

            //TODO: deal with outlet
            // outlefloat(this->confidence_outlet, confidence);
            // outlefloat(this->nearestDist_outlet, bestDist);
            // outlefloat(this->id_outlet, winningID);

            if (this->outputKnnMatches)
            {
                for (t_instanceIdx i = 0; i < this->kValue; ++i)
                {
                    // suppress reporting of the vote-winning id, because it was already reported above
                    if (this->instances[i].knnInfo.cluster != winningID)
                    {
                        //TODO: deal with outlet
                        // outlefloat(this->nearestDist_outlet, this->instances[i].knnInfo.safeDist);
                        // outlefloat(this->id_outlet, this->instances[i].knnInfo.cluster);
                    }
                }
            }
        }
        else
            tIDLib::logError("No training instances have been loaded. cannot perform ID.");
    }

    void worstMatch(const std::vector<float>& input)
    {
        float dist, worstDist;
        t_instanceIdx i, losingID;
        if (this->numInstances)
        {
            if (input.size() > this->maxFeatureLength)
            {
                tIDLib::logError("Input feature list too long. input ignored.");
                return;
            }

            for (i = 0; i < input.size(); ++i)
                this->attributeData[i].inputData = input[i];

            losingID = UINT_MAX;
            worstDist = -FLT_MAX;

            for (i = 0; i < this->numInstances; ++i)
            {
                dist = this->getInputDist(i);

                // abort _worstMatch() altogether if distance measurement fails
                if (dist == FLT_MAX)
                    return;

                if (dist > worstDist)
                {
                    worstDist = dist;
                    losingID = i;
                }
            }

            losingID = this->instances[losingID].clusterMembership;

            //TODO: deal with outlets
            // outlefloat(this->confidence_outlet, 0);
            // outlefloat(this->nearestDist_outlet, worstDist);
            // outlefloat(this->id_outlet, losingID);
        }
        else
            tIDLib::logError("No training instances have been loaded. cannot perform worst match.");
    }

    /* -------- END classification methods -------- */

    /* -------- concatenative synthesis methods -------- */

    /**
     * TODO:comment
     * Equivalent to sending data to the third inlet
    */
    void concatId(const std::vector<float>& input)
    {
        if (this->numInstances)
        {
            float dist, bestDist;
            t_instanceIdx i, j, halfNeighborhood, winningID;
            signed long int searchStart; // need this to be signed for wraparound
            t_attributeIdx listLength = input.size();

            // init knn info
            for (i = 0; i < this->numInstances; ++i)
            {
                this->instances[i].knnInfo.idx = i;
                this->instances[i].knnInfo.dist = this->instances[i].knnInfo.safeDist = FLT_MAX;
            }

            halfNeighborhood = this->neighborhood*0.5;

            if (listLength > this->maxFeatureLength)
            {
                tIDLib::logError("Input feature list too long. input ignored.");
                return;
            }

            for (i = 0; i < listLength; ++i)
                this->attributeData[i].inputData = input[i];

            winningID = UINT_MAX;
            bestDist = FLT_MAX;

            // searchStart needs to be signed in order to do wraparound. since t_instanceIdx is a unsigned int, we need a signed long int. unsigned int overflow is undefined, so can't count on that as an alternative for wraparound into negative
            if (this->concatWrap)
            {
                // for just the searchStart, check to see if this->neighborhood was EVEN.  if so, we should make searchStart = searchCenter - halfNeighborhood + 1
                if (this->neighborhood%2 == 0)
                    searchStart = this->searchCenter - halfNeighborhood + 1;
                else
                    searchStart = this->searchCenter - halfNeighborhood;

                if (searchStart < 0)
                    searchStart = this->numInstances + searchStart;  // wraps in reverse to end of table.  + searchStart because searchStart is negative here.
                else
                    searchStart = searchStart%this->numInstances;
            }
            else
            {
                // for just the searchStart, check to see if this->neighborhood was EVEN.  if so, we should make searchStart = searchCenter - halfNeighborhood + 1
                if (this->neighborhood%2 == 0)
                    searchStart = this->searchCenter - halfNeighborhood + 1;
                else
                    searchStart = this->searchCenter - halfNeighborhood;

                // TODO: is this (t_instanceIdx) typecast a good idea here? Just to get rid of warning?
                if (searchStart<0)
                    searchStart = 0; // no wrapping
                else if ((t_instanceIdx)searchStart >= this->numInstances)
                    searchStart = this->numInstances-1;
            }

            for (j = 0, i = searchStart; j < this->neighborhood; ++j)
            {
                dist = this->getInputDist(i);

                // abort _concat_id() altogether if distance measurement fails
                if (dist==FLT_MAX)
                    return;

                this->instances[i].knnInfo.dist = this->instances[i].knnInfo.safeDist = dist; // store the distance

                i++;

                if (this->concatWrap)
                    i = i%this->numInstances;
                else
                {
                    if (i>=this->numInstances)
                        break;
                }
            }

            // a reduced sort, so that the first maxMatches elements in this->instances will have the lowest distances in their knnInfo.safeDist, and in order to boot.
            // pass this->prevMatch to make sure we don't output the same match two times in a row (to prevent one grain being played back several
            // times in sequence.
            // this is wasteful in restricted searches because we don't need to look through all this->numInstances
            tIDLib::sortKnnInfo(this->maxMatches, this->numInstances, this->prevMatch, this->instances);

            if (this->prevMatch == UINT_MAX)
            {
                winningID = this->instances[0].knnInfo.idx;
                bestDist = this->instances[0].knnInfo.safeDist;
            }
            else
            {
                for (i = 0, bestDist = FLT_MAX; i < this->maxMatches; ++i)
                {
                    t_instanceIdx thisInstance;

                    thisInstance = this->instances[i].knnInfo.idx;

                    dist = this->getDist(this->instances[this->prevMatch], this->instances[thisInstance]);

                    // abort _concat_id() altogether if distance measurement fails
                    if (dist == FLT_MAX)
                        return;

                    if (dist < bestDist)
                    {
                        bestDist = dist;
                        winningID = thisInstance;
                    }
                }
            }

            if (this->reorientFlag)
                this->searchCenter = winningID;

            if (rand() < RAND_MAX*this->jumpProb)
            {
                winningID += rand();
                winningID = winningID%this->numInstances;
                this->searchCenter = winningID;
            }

            if (this->stutterProtect)
                this->prevMatch = winningID;
            else
                this->prevMatch = UINT_MAX;

            //TODO: deal with outlets
            // outlefloat(this->nearestDist_outlet, bestDist);
            // outlefloat(this->id_outlet, winningID);
        }
        else
            tIDLib::logError("No training instances have been loaded. cannot perform ID.");
    }

    /**
     * TODO:comment
    */
    void concatSearchWrap(bool flag)
    {
        this->concatWrap = flag;
    }

    /**
     * TODO:comment
    */
    void concatNeighborhood(t_instanceIdx n)
    {
        n = (n > this->numInstances) ? this->numInstances : n;
        n = (n < 1) ? 1 : n;
        this->neighborhood = n;
    }

    /**
     * TODO:comment
    */
    void concatJumpProb(float jp)
    {
        jp = (jp < 0) ? 0 : jp;
        jp = (jp > 1) ? 1 : jp;
        this->jumpProb = jp;
    }

    /**
     * TODO:comment
    */
    void concatReorient(bool r)
    {
        this->reorientFlag = r;
    }

    /**
     * TODO:comment
    */
    void concatSearchCenter(t_instanceIdx sc)
    {
        if (sc < 0)
            this->searchCenter = 0;
        else if (sc >=this->numInstances)
            this->searchCenter = this->numInstances-1;
        else
            this->searchCenter = sc;
    }

    /**
     * TODO:comment
    */
    void concatMaxMatches(t_instanceIdx mm)
    {
        mm = (mm < 1) ? 1 : mm;
        this->maxMatches = (mm > this->numInstances) ? this->numInstances : mm;
    }

    /**
     * TODO:comment
    */
    void concatStutterProtect(bool sp)
    {
        this->stutterProtect = sp;
    }
    /* -------- END concatenative synthesis methods -------- */

    /**
     * This is only relevant if clustering has been performed.
     * Change the number of nearest neighbors searched in the KNN algorithm
     * (default is 1). If knn is set to a value slightly higher than the number
     * of training examples you give per instrument, the confidence measure
     * reported from timbreID's 3rd outlet will be much more meaningful
    */
    void setK(t_instanceIdx k)
    {
        if (k < 1.0)
        {
            std::string errtxt = "K must be greater than one.";
            tIDLib::logError(errtxt);
            throw std::invalid_argument(errtxt);
        }

        if (k > this->numInstances)
        {
            std::string errtxt = "K must be less than the total number of instances.";
            tIDLib::logError(errtxt);
            throw std::invalid_argument(errtxt);
        }

        this->kValue = k;
        tIDLib::logInfo("Searching "+std::to_string(this->kValue)+" neighbors for KNN.");
    }

    /**
     * TODO:comment
    */
    void setOutputKnnMatches(bool out)
    {
        this->outputKnnMatches = out;

        if (this->outputKnnMatches)
            tIDLib::logInfo("Reporting all KNN matches.");
        else
            tIDLib::logInfo("Reporting best match only.");
    }

    /**
     * TODO:comment
    */
    void setNormalize(float n)
    {
        // create local memory
        std::vector<float> attributeColumn(this->numInstances);

        if (n <= 0)
        {
            // initialize normData
            for (t_attributeIdx i = 0; i < this->maxFeatureLength; ++i)
            {
                this->attributeData[i].normData.min = 0.0;
                this->attributeData[i].normData.max = 0.0;
                this->attributeData[i].normData.normScalar = 1.0;
            }

            this->normalize = false;
            tIDLib::logInfo("Eature attribute normalization OFF.");
        }
        else
        {
            if (this->numInstances)
            {
                // j for columns (attributes), i for rows (instances)
                for (t_attributeIdx j = 0; j < this->maxFeatureLength; ++j)
                {
                    for (t_instanceIdx i = 0; i < this->numInstances; ++i)
                    {
                        if (j > this->instances[i].length-1)
                        {
                            // TODO: is this a good way to escape?
                            tIDLib::logError("Attribute "+std::to_string(j)+" out of range for database instance "+std::to_string(i)+". aborting normalization");
                            this->normalize = false;
                            tIDLib::logInfo("Feature attribute normalization OFF.");
                            return;
                        }
                        else
                            attributeColumn[i] = this->instances[i].data[j];
                    }

                    std::sort(attributeColumn.begin(),attributeColumn.end());

                    this->attributeData[j].normData.min = attributeColumn[0];
                    this->attributeData[j].normData.max = attributeColumn[this->numInstances-1];

                    if (this->attributeData[j].normData.max <= this->attributeData[j].normData.min)
                    {
                        // this will fix things in the case of 1 instance, where min==max
                        this->attributeData[j].normData.min = 0.0;
                        this->attributeData[j].normData.normScalar = 1.0;
                    }
                    else
                        this->attributeData[j].normData.normScalar = 1.0/(this->attributeData[j].normData.max - this->attributeData[j].normData.min);

                }

                this->normalize=true;
                tIDLib::logInfo("Feature attribute normalization ON.");
            }
            else
                tIDLib::logError("No training instances have been loaded. cannot calculate normalization terms.");
        }
    }

    /**
     * TODO:comment
    */
    void manualCluster(t_instanceIdx numClusters, t_instanceIdx clusterIdx, t_instanceIdx low, t_instanceIdx hi)
    {
        t_instanceIdx i, j;

        if (low > hi)
        {
            t_instanceIdx tmp = low;
            low = hi;
            hi = tmp;
        }

        t_instanceIdx numMembers = hi - low + 1;

        if (low > this->numInstances-1 || hi > this->numInstances-1)
        {
            tIDLib::logError("Instances out of range. clustering failed.");
            return;
        }

        if (clusterIdx >= numClusters)
        {
            tIDLib::logError("Cluster index "+std::to_string(clusterIdx)+" out of range for given number of clusters "+std::to_string(numClusters)+". clustering failed.");
            return;
        }

        if (this->numInstances < numClusters)
        {
            tIDLib::logError("Not enough instances to cluster. clustering failed.");
            throw std::logic_error("Not enough instances to cluster. clustering failed.");
        }
        else
        {
            // update the number of clusters, trusting that the user knows what they're doing!
            this->numClusters = numClusters;

            // update the .numMembers value
            this->clusters[clusterIdx].numMembers = numMembers + 1; // +1 for the terminating UINT_MAX

            // get new memory for this cluster's members
            this->clusters[clusterIdx].members.resize(this->clusters[clusterIdx].numMembers);

            this->clusters[clusterIdx].votes = 0;

            for (i = low; i <= hi; ++i)
                this->instances[i].clusterMembership = clusterIdx;

            for (i = low, j = 0; i <= hi; ++i, ++j)
                this->clusters[clusterIdx].members[j] = i;

            // terminate with UINT_MAX
            this->clusters[clusterIdx].members[j] = UINT_MAX;

            // resize the excess clusterMembers memory back to the default size of 2 and store the default instance index as the cluster member followed by a terminating UINT_MAX
            for (i = this->numClusters; i < this->numInstances; ++i)
            {
                this->clusters[i].members.resize(2);
                this->clusters[i].numMembers = 2;
                this->clusters[i].members[0] = i;
                this->clusters[i].members[1] = UINT_MAX;
            }

            tIDLib::logInfo("Cluster "+std::to_string(clusterIdx)+" contains instances "+std::to_string(low)+" through "+std::to_string(hi)+".");
        }
    }

    /**
     * TODO:comment
    */
    std::vector<float> computeCluster(t_instanceIdx numClusters)
    {
        if (this->numInstances < numClusters)
            tIDLib::logError("Not enough instances to cluster.");
        else if (this->numClusters != this->numInstances)
            tIDLib::logError("Instances already clustered. uncluster first.");
        else if (numClusters==0)
            tIDLib::logError("Cannot create 0 clusters.");
        else
        {
            t_instanceIdx i, j, k, numInstances, numInstancesM1, numPairs, numClusterMembers1, numClusterMembers2, numClusterMembersSum, clusterCount;

            std::vector<t_instanceIdx> minDistIdx;
            std::vector<tIDLib::t_instance> clusterData;
            std::vector<float> pairDists;
            float minDist, numClusterMembers1_recip;
            std::vector<float> listOut;

            this->numClusters = numClusters;
            numInstances = this->numInstances;
            numInstancesM1 = numInstances-1;
            numPairs = (numInstances*numInstancesM1) * 0.5;
            clusterCount = numInstances;
            numClusterMembers1 = 0;
            numClusterMembers2 = 0;
            numClusterMembers1_recip = 1;
            i=j=k=0;

            // Resize local memory vectors
            minDistIdx.resize(2);
            clusterData.resize(numInstances);
            pairDists.resize(numPairs);
            listOut.resize(numInstances);

            for (i=0; i<numInstances; ++i)
            {
                this->clusters[i].members[0] = i; // first member of the cluster is the instance index
                this->clusters[i].members[1] = UINT_MAX;
            }

            // copy this->instances into a safe local copy: clusterData
            for (i=0; i<numInstances; ++i)
            {
                clusterData[i].length = this->instances[i].length;
                clusterData[i].data.resize(this->instances[i].length);

                for (j=0; j<clusterData[i].length; ++j)
                    clusterData[i].data[j] = this->instances[i].data[j];
            }

            while (clusterCount > this->numClusters)
            {
                minDist = FLT_MAX;

                // init minDistIdx to UINT_MAX as an indicator that it's initialized
                for (i=0; i<2; ++i)
                    minDistIdx[i] = UINT_MAX;

                // init pair distances
                for (i=0; i<numPairs; ++i)
                    pairDists[i] = FLT_MAX;

                // get distances between all possible pairs in clusterData
                for (i=0, k=0; i<numInstancesM1; ++i)
                {
                    if (clusterData[i].data[0] != FLT_MAX) // if this is true, the data hasn't been clustered yet.
                    {
                        for (j=1; j<numInstances; ++j)
                        {
                            if ((i+j) < numInstances)
                            {
                                if (clusterData[i+j].data[0] != FLT_MAX)
                                {
                                    pairDists[k] = this->getDist(clusterData[i], clusterData[i+j]);
                                    numClusterMembers1 = this->clusters[i].numMembers-1; // -1 because the list is terminated with UINT_MAX
                                    numClusterMembers2 = this->clusters[i+j].numMembers-1;

                                    // definition of Ward's linkage from MATLAB linkage doc
                                    // pairDists[k] is already euclidean distance
                                    numClusterMembersSum = numClusterMembers1 + numClusterMembers2;

                                    if (numClusterMembersSum > 0)
                                        pairDists[k] = numClusterMembers1*numClusterMembers2 * (pairDists[k]/(numClusterMembers1+numClusterMembers2));
                                    else
                                        pairDists[k] = FLT_MAX;

                                    if (pairDists[k]<minDist)
                                    {
                                        minDist=pairDists[k];
                                        minDistIdx[0]=i;
                                        minDistIdx[1]=i+j;
                                    }

                                    k++; // increment pairDists index if something was actually written to it.
                                }
                            }
                            else
                                break;
                        }
                    }
                }

                // we've found the smallest distance between clusterData elements and stored it
                // in minDist. we've store the clusterData indices of the two elements in
                // minDistIdx[0] and minDistIdx[1].

                // set i to the index for storing the new member(s) of the cluster.
                i = this->clusters[minDistIdx[0]].numMembers-1;

                // actually store the new member(s).
                j=0;
                while (this->clusters[minDistIdx[1]].members[j] != UINT_MAX)
                {
                    // make some more memory for the new member(s)
                    this->clusters[minDistIdx[0]].members.resize(this->clusters[minDistIdx[0]].numMembers + 1);
                    (this->clusters[minDistIdx[0]].numMembers)++; // remember to update this member list's length
                    this->clusters[minDistIdx[0]].members[i++] = this->clusters[minDistIdx[1]].members[j++];
                }

                i = this->clusters[minDistIdx[0]].numMembers-1;
                this->clusters[minDistIdx[0]].members[i] = UINT_MAX; // terminate with UINT_MAX

                numClusterMembers1 = this->clusters[minDistIdx[0]].numMembers-1;

                if (numClusterMembers1 > 0)
                    numClusterMembers1_recip = 1.0/(float)numClusterMembers1;
                else
                    numClusterMembers1_recip = 1.0;

                // resize the usurped cluster's cluster list memory, and update its size to 1
                this->clusters[minDistIdx[1]].members.resize(1);
                this->clusters[minDistIdx[1]].members[0] = UINT_MAX; // terminate with UINT_MAX
                this->clusters[minDistIdx[1]].numMembers = 1;

                // grab the first original instance for this cluster index
                for (i=0; i<clusterData[minDistIdx[0]].length; ++i)
                    clusterData[minDistIdx[0]].data[i] = this->instances[minDistIdx[0]].data[i];

                // sum the original instances of the cluster members to compute centroid below
                for (i=1; i<numClusterMembers1; ++i)
                    for (j=0; j<clusterData[minDistIdx[0]].length; ++j)
                        clusterData[minDistIdx[0]].data[j] += this->instances[  this->clusters[minDistIdx[0]].members[i]  ].data[j];

                // compute centroid
                for (i=0; i<clusterData[minDistIdx[0]].length; ++i)
                    clusterData[minDistIdx[0]].data[i] *= numClusterMembers1_recip;

                // write FLT_MAX to the first element in the nearest neighbor's instance to indicate it's now vacant.
                // this is all that's needed since all previous members were averaged and stored here.
                clusterData[minDistIdx[1]].data[0] = FLT_MAX;

                clusterCount--;
            }

            // since the indices of the clusters have gaps from the process,
            // shift the clusterMembers arrays that actually have content (!= UINT_MAX)
            // to the head of clusterMembers.  this will produce indices from 0 through numClusters-1.
            for (i=0, k=0; i<numInstances; ++i)
                if (this->clusters[i].members[0] != UINT_MAX)
                {
                    // resize this member list
                    this->clusters[k].members.resize(this->clusters[i].numMembers);

                    for (j=0; j<this->clusters[i].numMembers; ++j)
                        this->clusters[k].members[j] = this->clusters[i].members[j];

                    // shift the list length info back
                    this->clusters[k].numMembers = this->clusters[i].numMembers;

                    k++;
                }

            // resize the excess clusterMembers memory back to the default size of 2 and store the default instance index as the cluster member followed by a terminating UINT_MAX
            for (i=this->numClusters; i<numInstances; ++i)
            {
                this->clusters[i].members.resize(2);
                this->clusters[i].numMembers = 2;
                this->clusters[i].members[0] = i;
                this->clusters[i].members[1] = UINT_MAX;
            }

            // UPDATE: as of version 0.7, not doing this resize because we're always keeping this->clusters to be size this->numInstances
            // resize clusterMembers so it is only this->numClusters big
            // this->clusters.resize(this->numClusters);

            for (i=0, k=0; i<this->numClusters; ++i)
            {
                for (j=0; j<(this->clusters[i].numMembers-1); ++j, ++k)
                {
                    this->instances[this->clusters[i].members[j]].clusterMembership = i;
                    listOut[k] = this->clusters[i].members[j];
                }
            }

            tIDLib::logInfo("Instances clustered.");
            return listOut;
        }
    }

    /**
     * TODO:comment
    */
    void uncluster()
    {
        t_instanceIdx i;

        // free each this->clusters list's memory
        for (i=0; i<this->numClusters; ++i)
        {
            this->clusters[i].members.clear();
            this->clusters[i].members.resize(2);
        }

        this->numClusters = this->numInstances;

        for (i=0; i<this->numInstances; ++i)
        {
            this->instances[i].clusterMembership = i; // init membership to index
            this->clusters[i].members[0] = i; // first member of the cluster is the instance index
            this->clusters[i].members[1] = UINT_MAX; // terminate cluster member list with UINT_MAX
            this->clusters[i].numMembers = 2;
            this->clusters[i].votes = 0;
        }

        tIDLib::logInfo("Instances unclustered.");
    }

    /**
     *
     * TODO:comment
     * Order attributes by variance, so that only the most relevant attributes can
     * be used to calculate the distance measure. For instance, after ordering a
     * 47-component BFCC vector by variance, you may only want to compare the first
     * 10 attributes, since those will be the ones with the most variance. Specify
     * this range using the timbreID_attributeRange method.
     * Like "normalize", this is not an operation to try during real time
     * performance.
    */
    void orderAttributesByVariance()
    {
        if (this->numInstances > 0)
        {
            t_instanceIdx i, j;

            // create local memory
            std::vector<float> attributeVar(this->maxFeatureLength);
            std::vector<tIDLib::t_instance> meanCentered(this->numInstances);

            for (i=0; i<this->numInstances; ++i)
            {
                meanCentered[i].length = this->instances[i].length;
                meanCentered[i].data.resize(meanCentered[i].length);
            }

            // init mean centered
            for (i=0; i<this->numInstances; ++i)
                for (j=0; j<meanCentered[i].length; ++j)
                    meanCentered[i].data[j] = 0.0;

            // get the mean of each attribute
            for (i=0; i<this->maxFeatureLength; ++i)
                attributeVar[i] = this->attributeMean(this->numInstances, i, this->instances, this->normalize, this->attributeData);

            // center the data and write the matrix B
            for (i=0; i<this->numInstances; ++i)
                for (j=0; j<meanCentered[i].length; ++j)
                {
                    if (this->normalize)
                        meanCentered[i].data[j] = ((this->instances[i].data[j] - this->attributeData[j].normData.min) * this->attributeData[j].normData.normScalar) - attributeVar[j];
                    else
                        meanCentered[i].data[j] = this->instances[i].data[j] - attributeVar[j];
                }

            // variance is calculated as: sum(B(:,1).^2)/(M-1) for the first attribute
            // run process by matrix columns rather than rows, hence the j, i order
            for (j=0; j<this->maxFeatureLength; ++j)
            {
                attributeVar[j] = 0;

                for (i=0; i<this->numInstances; ++i)
                {
                    if (j<meanCentered[i].length)
                        attributeVar[j] += meanCentered[i].data[j] * meanCentered[i].data[j];
                }

                if ((this->numInstances-1) > 0)
                    attributeVar[j] /= this->numInstances-1;
            }

            // sort attributeOrder by largest variances: find max in attributeVar,
            // replace it with -FLT_MAX, find next max.
            for (i=0; i<this->maxFeatureLength; ++i)
            {
                float max = 0.0;
                for (j=0; j<this->maxFeatureLength; ++j)
                {
                    if (attributeVar[j] > max)
                    {
                        max = attributeVar[j];
                        this->attributeData[i].order = j;
                    }
                }

                attributeVar[this->attributeData[i].order] = -FLT_MAX;
            }

            tIDLib::logInfo("Attributes ordered by variance.");
        }
        else
        {
            tIDLib::logError("No instances for variance computation.");
            throw std::logic_error("No instances for variance computation.");
        }
    }

    /**
     * TODO:comment
    */
    void setRelativeOrdering(bool rel)
    {
        this->relativeOrdering = rel;

        if (this->relativeOrdering)
            tIDLib::logInfo("Relative ordering ON.");
        else
            tIDLib::logInfo("Relative ordering OFF.");
    }

    /**
     * TODO:comment
    */
    void setDistMetric(DistanceMetric d)
    {
        this->distMetric = d;

        switch(this->distMetric)
        {
            case DistanceMetric::euclidean:
                tIDLib::logInfo("Distance metric: EUCLIDEAN.");
                break;
            case DistanceMetric::taxi:
                tIDLib::logInfo("Distance metric: MANHATTAN (taxicab distance).");
                break;
            case DistanceMetric::correlation:
                tIDLib::logInfo("Distance metric: PEARSON CORRELATION COEFF.");
                break;
            default:
                break;
        }
    }

    /**
     * TODO:comment
    */
    void setWeights(std::vector<float> weights)
    {
        t_attributeIdx i;
        t_attributeIdx listLength = weights.size();

        if (listLength > this->maxFeatureLength)
        {
            tIDLib::logError("Weights list longer than current feature length. ignoring excess weights");
            listLength = this->maxFeatureLength;
        }

        for (i=0; i<listLength; ++i)
            this->attributeData[i].weight = weights[i];

        // if only the first few of a long feature vector are specified, fill in the rest with 1.0
        for (; i<this->maxFeatureLength; ++i)
            this->attributeData[i].weight = 1.0;
    }

    /**
     * You can provide a list of symbolic names for attributes in your feature
     * vector.
     * Attributes are named "NULL" by default.
     */
    void setAttributeNames(std::vector<std::string> names)
    {
        t_attributeIdx i, listLength;

        listLength = names.size();

        if (listLength > this->maxFeatureLength)
        {
            tIDLib::logError("Attribute list longer than current max feature length. ignoring excess attributes");
            listLength = this->maxFeatureLength;
        }

        for (i=0; i<listLength; ++i)
            this->attributeData[i].name = names[i];

        tIDLib::logInfo("Attribute names received.");
    }

    /**
     *
     * Manually order the attributes. In conjunction with attribute_range, this is
     * useful for exporting specific subsets of attributes, or doing distance
     * calculation based on subsets of attributes
    */
    void setAttributeOrder(std::vector<t_attributeIdx> order)
    {
        t_attributeIdx i, j, listLength;

        listLength = order.size();

        if (listLength > this->maxFeatureLength)
        {
            tIDLib::logError("Attribute list longer than current max feature length. ignoring excess attributes");
            listLength = this->maxFeatureLength;
        }

        for (i=0; i<listLength; ++i)
        {
            this->attributeData[i].order = order[i];

            if (this->attributeData[i].order>this->maxFeatureLength-1)
            {
                // initialize attributeOrder
                for (j=0; j<this->maxFeatureLength; ++j)
                    this->attributeData[j].order = j;

                tIDLib::logError("Attribute "+std::to_string(this->attributeData[i].order)+" out of range. attribute order initialized.");
            }
        }

        // fill any remainder with attribute 0
        for (; i<this->maxFeatureLength; ++i)
            this->attributeData[i].order = 0;

        tIDLib::logInfo("Attributes re-ordered.");
    }

    /**
     *
     * Specify a restricted range of attributes to use in the distance calculation.
     */
    void setAttributeRange(t_attributeIdx lo, t_attributeIdx hi)
    {
        this->attributeLo = (lo<0)?0:lo;
        this->attributeLo = (this->attributeLo>=this->maxFeatureLength)?this->maxFeatureLength-1:this->attributeLo;

        this->attributeHi = (hi<0)?0:hi;
        this->attributeHi = (this->attributeHi>=this->maxFeatureLength)?this->maxFeatureLength-1:this->attributeHi;

        if (this->attributeLo>this->attributeHi)
        {
            t_attributeIdx tmp;
            tmp = this->attributeHi;
            this->attributeHi = this->attributeLo;
            this->attributeLo = tmp;
        }

        tIDLib::logInfo("Attribute range: "+std::to_string(this->attributeLo)+" through "+std::to_string(this->attributeHi)+".");
    }

    /**
     *
     * Go back to regular attribute order.
    */
    void reorderAttributes()
    {
        // initialize attributeOrder
        for (t_attributeIdx i = 0; i < this->maxFeatureLength; ++i)
            this->attributeData[i].order = i;

        tIDLib::logInfo("Attribute order initialized.");
    }

    /**
     *
     * Wipe all instances to start over.
    */
    void clearAll()
    {
        t_instanceIdx i;

        // uncluster first
        this->uncluster();

        // clear each instance's data memory
        for (i=0; i<this->numInstances; ++i)
            this->instances[i].data.clear();

        this->instances.clear();

        // free each cluster's members memory
        for (i=0; i<this->numClusters; ++i)
            this->clusters[i].members.clear();

        this->clusters.clear();

        // postFlag argument FALSE here
        this->attributeDataResize(0, false);

        this->numInstances = 0;
        this->numClusters = 0;
        this->neighborhood = 0;
        this->searchCenter = 0;

        this->maxFeatureLength = 0;
        this->minFeatureLength = INT_MAX;
        this->normalize = false;

        tIDLib::logInfo("All instances cleared.");
    }

    /**
     * TimbreID's default binary file format is .timid. This will write and read
     * much faster than the text format, so it's the best choice for large databases
     * (i.e. 1000s of instances)
     * eg. writeData("./data/feature-db.timid")
     * If you need to look at the feature database values, you might want to export
     * in the text file format using the writeText method
     * @see writeTextData
    */
    void writeData(std::string filename)
    {
        FILE *filePtr;

        // make a buffer for the header data, which is the number of instances followed by the length of each instance
        std::vector<t_instanceIdx> header(this->numInstances+1);

        filePtr = fopen(filename.c_str(), "wb");

        if (!filePtr)
        {
            tIDLib::logError("Failed to create " + filename);
            return;
        }

        // write the header information, which is the number of instances, and then the length of each instance
        header[0] = this->numInstances;
        for (t_instanceIdx i = 0; i < this->numInstances; ++i)
            header[i+1] = this->instances[i].length;

        fwrite(&(header[0]), sizeof(t_instanceIdx), this->numInstances+1, filePtr);

        // write the instance data
        for (t_instanceIdx i = 0; i < this->numInstances; ++i)
            fwrite(&(this->instances[i].data[0]), sizeof(float), this->instances[i].length, filePtr);

        tIDLib::logInfo("Wrote "+std::to_string(this->numInstances)+" non-normalized instances to "+filename+".");

        fclose(filePtr);
    }

    /*
     * TimbreID's default binary file format is .timid. This will write and read
     * much faster than the text format, so it's the best choice for large databases
     * (i.e. 1000s of instances)
     * eg. writeData("./data/feature-db.timid")
    */
    void readData(std::string filename)
    {
        FILE *filePtr;
        t_instanceIdx i;

        filePtr = fopen(filename.c_str(), "rb");

        if (!filePtr)
        {
            tIDLib::logError("Failed to open " + filename);
            return;
        }

        t_instanceIdx maxLength = 0;
        t_instanceIdx minLength = INT_MAX;

        // erase old instances & clusters and resize to 0. this also does a sub-call to this->attributeDataResize()
        this->clearAll();

        // first item in the header is the number of instances
        fread(&this->numInstances, sizeof(t_instanceIdx), 1, filePtr); //TODO check if this is working (the cast to pointer of the first vector element)

        // resize instances & clusterMembers to numInstances
        this->instances.resize(this->numInstances);
        this->clusters.resize(this->numInstances);

        for (i=0; i<this->numInstances; ++i)
        {
            // get the length of each instance
            fread(&this->instances[i].length, sizeof(t_attributeIdx), 1, filePtr);

            if (this->instances[i].length>maxLength)
                maxLength = this->instances[i].length;

            if (this->instances[i].length<minLength)
                minLength = this->instances[i].length;

            // get the appropriate number of bytes for the data
            this->instances[i].data.resize(this->instances[i].length);
        }

        this->minFeatureLength = minLength;
        this->maxFeatureLength = maxLength;
        this->neighborhood = this->numInstances;
        this->numClusters = this->numInstances;

        // update this->attributeData based on new this->maxFeatureLength. turn postFlag argument TRUE
        this->attributeDataResize(this->maxFeatureLength, 1);

        // after loading a database, instances are unclustered
        for (i=0; i<this->numInstances; ++i)
        {
            this->clusters[i].numMembers = 2;
            this->clusters[i].members.resize(this->clusters[i].numMembers);

            this->clusters[i].members[0] = i; // first member of the cluster is the instance index
            this->clusters[i].members[1] = UINT_MAX; // terminate with UINT_MAX

            this->instances[i].clusterMembership = i; // init instance's cluster membership to index
        }

        // finally, read in the instance data
        for (i=0; i<this->numInstances; ++i)
            fread(&(this->instances[i].data[0]), sizeof(float), this->instances[i].length, filePtr);

        tIDLib::logInfo("Read "+std::to_string(this->numInstances)+" instances from "+filename+".");
        fclose(filePtr);
    }

    /**
     * When needing to look at the feature database values, data can be exported
     * in the text file format using this method
    */
    void writeTextData(std::string filename)
    {
        FILE *filePtr;
        t_instanceIdx i;
        t_attributeIdx j;

        filePtr = fopen(filename.c_str(), "w");

        if (!filePtr)
        {
            tIDLib::logError("Failed to create " + filename);
            return;
        }

        for (i=0; i<this->numInstances; ++i)
        {
            j=this->attributeLo;

            while (j<=this->attributeHi)
            {
                float thisFeatureData;
                t_attributeIdx thisAttribute;

                thisAttribute = this->attributeData[j].order;

                if (thisAttribute>=this->instances[i].length)
                    break;

                if (this->normalize)
                    thisFeatureData = (this->instances[i].data[thisAttribute] - this->attributeData[thisAttribute].normData.min)*this->attributeData[thisAttribute].normData.normScalar;
                else
                    thisFeatureData = this->instances[i].data[thisAttribute];

                // What's the best float resolution to print?
                // no space after final value on an instance line
                if (j==this->attributeHi)
                    fprintf(filePtr, "%0.6f", thisFeatureData);
                else
                    fprintf(filePtr, "%0.6f ", thisFeatureData);

                j++;
            }

            fprintf(filePtr, "\n");
        }

        tIDLib::logInfo("Wrote "+std::to_string(this->numInstances)+" instances to " + filename + ".");
        fclose(filePtr);
    }

    /**
     * Used to read text file format
    */
    void readTextData(std::string filename)
    {
        FILE *filePtr;
        t_instanceIdx i, j, numInstances, stringLength, numSpaces;
        char textLine[tIDLib::MAXTIDTEXTSTRING];

        filePtr = fopen(filename.c_str(), "r");

        if (!filePtr)
        {
            tIDLib::logError("Failed to open " + filename);
            return;
        }

        numInstances = 0;
        while (fgets(textLine, tIDLib::MAXTIDTEXTSTRING, filePtr))
            numInstances++;

        t_instanceIdx maxLength = 0;
        t_instanceIdx minLength = INT_MAX;

        // erase old instances & clusters and resize to 0. this also does a sub-call to this->attributeDataResize()
        this->clearAll();

        this->numInstances = numInstances;

        // resize instances & clusterMembers to numInstances
        this->instances.resize(this->numInstances);
        this->clusters.resize(this->numInstances);

        // now that we have numInstances, close and re-open file to get the length of each line
        fclose(filePtr);
        filePtr = fopen(filename.c_str(), "r");

        i=0;
        while (fgets(textLine, tIDLib::MAXTIDTEXTSTRING, filePtr))
        {
            stringLength = strlen(textLine);

            // check to see if there's a space after the last data entry on the line. if so, our space counter loop below should stop prior to that final space. this allows us to read text files written both with and without spaces after the final entry of a line
            // stringLength-2 would be the position for this space, because the final character is a carriage return (10) at position stringLength-1
            if (textLine[stringLength-2]==32)
                stringLength = stringLength-2; // lop off the final space and CR

            numSpaces = 0;
            for (j=0; j<stringLength; ++j)
                if (textLine[j]==32)
                    numSpaces++;

            // there's a space after each entry in a file written by write_text(), except for the final entry. So (numSpaces+1)==length
            this->instances[i].length = numSpaces+1;

            if (this->instances[i].length>maxLength)
                maxLength = this->instances[i].length;

            if (this->instances[i].length<minLength)
                minLength = this->instances[i].length;

            // get the appropriate number of bytes for the data
            this->instances[i].data.resize(this->instances[i].length);

            i++;
        }

        this->minFeatureLength = minLength;
        this->maxFeatureLength = maxLength;
        this->neighborhood = this->numInstances;
        this->numClusters = this->numInstances;

        // update this->attributeData based on new this->maxFeatureLength. postFlag argument TRUE here
        this->attributeDataResize(this->maxFeatureLength, 1);

        for (i=0; i<this->numInstances; ++i)
        {
            this->clusters[i].numMembers = 2;
            this->clusters[i].members.resize(this->clusters[i].numMembers);

            this->clusters[i].members[0] = i; // first member of the cluster is the instance index
            this->clusters[i].members[1] = UINT_MAX;

            this->instances[i].clusterMembership = i; // init instance's cluster membership to index
        }

        // now that we have the number of instances and the length of each feature, close and re-open file to extract the actual data
        fclose(filePtr);
        filePtr = fopen(filename.c_str(), "r");

        for (i=0; i<this->numInstances; ++i)
        {
            for (j=0; j<this->instances[i].length; ++j)
                fscanf(filePtr, "%f", &this->instances[i].data[j]);
        }

        tIDLib::logInfo("Read "+std::to_string(this->numInstances)+" instances from " + filename + ".");
        fclose(filePtr);
    }

    /**
     * For small training datasets like the one in the original help patch,
     * clustering is basically instantaneous. But when working with large sets
     * (1000s of instances) like those in the timbre-order patches from the timbreID
     * example package, it can take several seconds or even minutes. In those cases,
     * it's good to to save the cluster information just computed. The fastest way
     * to read/write the data is with the readClusters & writeClusters methods
     * eg. writeClusters("./data/cluster.clu")
     */
    void writeClusters(std::string filename)
    {
        FILE *filePtr;
        t_instanceIdx i;

        filePtr = fopen(filename.c_str(), "wb");

        if (!filePtr)
        {
            tIDLib::logError("Failed to create " + filename);
            return;
        }

        // write a header indicating the number of clusters (this->numClusters)
        fwrite(&this->numClusters, sizeof(t_instanceIdx), 1, filePtr);

        // write the cluster memberships next
        for (i=0; i<this->numInstances; ++i)
            fwrite(&this->instances[i].clusterMembership, sizeof(t_instanceIdx), 1, filePtr);

        // write the number of members for each cluster
        for (i=0; i<this->numInstances; ++i)
            fwrite(&this->clusters[i].numMembers, sizeof(t_instanceIdx), 1, filePtr);

        // finally, write the members data
        for (i=0; i<this->numInstances; ++i)
            fwrite(&(this->clusters[i].members[0]), sizeof(t_instanceIdx), this->clusters[i].numMembers, filePtr);

        tIDLib::logInfo("Wrote "+std::to_string(this->numClusters)+" clusters to " + filename + ".");
        fclose(filePtr);
    }

    /**
     * For small training datasets like the one in the original help patch,
     * clustering is basically instantaneous. But when working with large sets
     * (1000s of instances) like those in the timbre-order patches from the timbreID
     * example package, it can take several seconds or even minutes. In those cases,
     * it's good to to save the cluster information just computed. The fastest way
     * to read/write the data is with the readClusters & writeClusters methods
     * eg. readClusters("./data/cluster.clu")
     */
    void readClusters(std::string filename)
    {
        FILE *filePtr;
        t_instanceIdx i, numClusters;

        filePtr = fopen(filename.c_str(), "rb");

        if (!filePtr)
        {
            tIDLib::logError("Failed to open " + filename);
            return;
        }

        // read header indicating number of clusters
        fread(&numClusters, sizeof(t_instanceIdx), 1, filePtr);

        if (numClusters>this->numInstances)
        {
            tIDLib::logError(filename + " contains more clusters than current number of database instances. read failed.");
            fclose(filePtr);
            return;
        }

        this->numClusters = numClusters;

        // read the cluster memberships next
        for (i=0; i<this->numInstances; ++i)
            fread(&this->instances[i].clusterMembership, sizeof(t_instanceIdx), 1, filePtr);

        // free members memory, read the number of members and get new members memory
        for (i=0; i<this->numInstances; ++i)
        {
            this->clusters[i].members.clear();
            fread(&this->clusters[i].numMembers, sizeof(t_instanceIdx), 1, filePtr);
            this->clusters[i].members.resize(this->clusters[i].numMembers);
        }

        // read the actual members data
        for (i=0; i<this->numInstances; ++i)
            fread(&(this->clusters[i].members[0]), sizeof(t_instanceIdx), this->clusters[i].numMembers, filePtr);

        tIDLib::logInfo("Read "+std::to_string(this->numClusters)+" clusters from "+filename+".");
        fclose(filePtr);
    }

    /**
     * The text format that results from "writeClustersText" takes a bit more
     * time to generate, but could be useful in certain situations. For
     * instance, even with small datasets, you might want to cluster data in an
     * unusual way, or maybe the automatic clustering algorithm just didn't work
     * the way you wanted it to. With these messages you can load cluster
     * information that you write yourself in a text file. To see the
     * appropriate format, save cluster info from this help patch and open up
     * the resulting text file. It couldn't be much simpler - each line contains
     * the index of instances that should be clustered together.
     * Terminate each line with a carriage return
    */
    void writeClustersText(std::string filename)
    {
        FILE *filePtr;
        t_instanceIdx i, j;

        filePtr = fopen(filename.c_str(), "w");

        if (!filePtr)
        {
            tIDLib::logError("Failed to create " + filename);
            return;
        }

        for (i=0; i<this->numClusters; ++i)
        {
            for (j=0; j<this->clusters[i].numMembers-2; ++j)
                fprintf(filePtr, "%i ", this->clusters[i].members[j]);

            // no space for the final instance given on the line for cluster i
            fprintf(filePtr, "%i", this->clusters[i].members[this->clusters[i].numMembers-1]); //TODO: check again if correct

            // newline to end the list of instances for cluster i
            fprintf(filePtr, "\n");
        }

        tIDLib::logInfo("Wrote "+std::to_string(this->numClusters)+" clusters to "+filename+".");
        fclose(filePtr);
    }

    /**
     * Read text format cluster data saved
    */
    void readClustersText(std::string filename)
    {
        FILE *filePtr;
        t_instanceIdx i, j, stringLength, numSpaces, numClusters;
        char textLine[tIDLib::MAXTIDTEXTSTRING];

        filePtr = fopen(filename.c_str(), "r");

        if (!filePtr)
        {
            tIDLib::logError("Failed to open " + filename);
            return;
        }

        // count lines in text file to determine numClusters
        numClusters = 0;
        while (fgets(textLine, tIDLib::MAXTIDTEXTSTRING, filePtr))
            numClusters++;

        if (numClusters>this->numInstances)
        {
            tIDLib::logError(filename + " contains more clusters than current number of database instances. read failed.");
            fclose(filePtr);
            return;
        }

        this->numClusters = numClusters;

        for (i=0; i<this->numInstances; ++i)
            this->clusters[i].members.clear();

        fclose(filePtr);
        filePtr = fopen(filename.c_str(), "r");

        i=0;
        while (fgets(textLine, tIDLib::MAXTIDTEXTSTRING, filePtr))
        {
            stringLength = strlen(textLine);

            // check to see if there's a space after the last data entry on the line. if so, our space counter loop below should stop prior to that final space. this allows us to read text files written both with and without spaces after the final entry of a line
            // stringLength-2 would be the position for this space, because the final character is a carriage return (10) at position stringLength-1
            if (textLine[stringLength-2]==32)
                stringLength = stringLength-2; // lop off the final space and CR

            numSpaces = 0;
            for (j=0; j<stringLength; ++j)
                if (textLine[j]==32)
                    numSpaces++;

            // there's a space after each entry in a file written by write_clusters_text(), except for the final instance on the line. So numMembers should be numSpaces+1. However, we must add one more slot for the terminating UINT_MAX of each cluster list. So in the end, numMembers should be numSpaces+2
            this->clusters[i].numMembers = numSpaces+2;

            // get the appropriate number of bytes for the data
           this->clusters[i].members.resize(this->clusters[i].numMembers);

            i++;
        }

        fclose(filePtr);
        filePtr = fopen(filename.c_str(), "r");

        for (i=0; i<this->numClusters; ++i)
        {
            for (j=0; j<this->clusters[i].numMembers-1; ++j)
                fscanf(filePtr, "%u", &(this->clusters[i].members[j]));

            // don't forget to terminate with UINT_MAX
            this->clusters[i].members[j] = UINT_MAX;
        }

        // fill out any remaining with the default setup
        for (; i<this->numInstances; ++i)
        {
            this->clusters[i].numMembers = 2;
            this->clusters[i].members.resize(this->clusters[i].numMembers);
            this->clusters[i].members[0] = i;
            this->clusters[i].members[1] = UINT_MAX;

            // set cluster membership to instance index
            this->instances[i].clusterMembership = i;
        }

        for (i=0; i<this->numClusters; ++i)
        {
            j=0;
            while (this->clusters[i].members[j] != UINT_MAX)
            {
                t_instanceIdx idx;
                idx = this->clusters[i].members[j];
                this->instances[idx].clusterMembership = i;
                j++;
            }
        }

        tIDLib::logInfo(filename + ": read "+std::to_string(this->numClusters)+" clusters from " + filename);
        fclose(filePtr);
    }

    /**
     * Return a string containing the main parameters of the module.
     * Refer to the PD helper files of the original timbreID library to know more:
     * https://github.com/wbrent/timbreID/tree/master/help
     * @return string with parameter info
    */
    std::string getInfoString() const noexcept
    {
        std::string res = "";
        res += "timbreID version ";
        res += TIDVERSION;

        res += "\nno. of instances: "+std::to_string(this->numInstances);
        res += "\nmax feature length: "+std::to_string(this->maxFeatureLength);
        res += "\nmin feature length: "+std::to_string(this->minFeatureLength);
        res += "\nattribute range: "+std::to_string(this->attributeLo)+" through "+std::to_string(this->attributeHi);
        res += "\nnormalization: "+std::to_string(this->normalize);
        res += "\ndistance metric: ";
        switch(this->distMetric)
        {
            case DistanceMetric::euclidean:
                res += "Euclidean distance";
                break;
            case DistanceMetric::taxi:
                res += "Manhattan distance";
                break;
            case DistanceMetric::correlation:
                res += "Pearson Correlation Coefficient";
                break;
            default:
                break;
        }

        res += "\nno. of clusters: "+std::to_string(this->numClusters);
        res += "\nKNN: "+std::to_string(this->kValue);
        res += "\noutput KNN matches: "+std::to_string(this->outputKnnMatches);
        res += "\nrelative ordering: "+std::to_string(this->relativeOrdering);
        res += "\nconcatenative wraparound: "+std::to_string(this->concatWrap);
        res += "\nsearch center: "+std::to_string(this->searchCenter);
        res += "\nneighborhood: "+std::to_string(this->neighborhood);
        res += "\nreorient: "+std::to_string(this->reorientFlag);
        res += "\nmax matches: "+std::to_string(this->maxMatches);
        res += "\njump probability: "+std::to_string(this->jumpProb);
        res += "\nstutter protect: "+std::to_string(this->stutterProtect);
        //TODO: Mabe log post errors and warnings to a file and print here the file path
        return res;
    }

    /** Information functions (Fourth outlet data in Puredata Version) -------*/

    /**
     * TODO:comment
    */
    t_instanceIdx getNumInstances() const
    {
        return this->numInstances;
    }

    /**
     * TODO:comment
    */
    std::vector<float> getFeatureList(t_instanceIdx idx) const
    {
        idx = (idx < 0) ? 0 : idx;

        // create local memory
        std::vector<float> listOut;

        if (idx > this->numInstances-1)
        {
            tIDLib::logError("Instance "+std::to_string(idx)+" does not exist.");
            throw std::logic_error("Instance "+std::to_string(idx)+" does not exist.");
        }
        else
        {
            t_attributeIdx thisFeatureLength = this->instances[idx].length;
            listOut.resize(thisFeatureLength);

            for (t_attributeIdx i = 0; i < thisFeatureLength; ++i)
            {
                if (this->normalize)
                    listOut[i] = (this->instances[idx].data[i] - this->attributeData[i].normData.min)*this->attributeData[i].normData.normScalar;
                else
                    listOut[i] = this->instances[idx].data[i];
            }
        }
        return listOut;
    }

    /**
     * TODO:comment
    */
    t_instanceIdx getClusterMembership(t_instanceIdx idx) const
    {
        if (idx >= this->numInstances)
        {
            tIDLib::logError("Instance "+std::to_string(idx)+" does not exist.");
            throw std::logic_error("Instance "+std::to_string(idx)+" does not exist.");
        }
        else
        {
            return this->instances[idx].clusterMembership;
        }
    }

    /**
     * TODO:comment
    */
    std::vector<float>  getClustersList() const
    {
        // create local memory
        std::vector<float> listOut(this->numInstances);

        for (t_instanceIdx i = 0, k = 0; i < this->numClusters; ++i)
            for (t_instanceIdx j = 0; j < (this->clusters[i].numMembers-1); ++j, ++k) // -1 because it's terminated by UINT_MAX
                listOut[i] = this->clusters[i].members[j];
        return listOut;
    }

    /**
     * TODO:comment
    */
    std::vector<float> clusterList(t_instanceIdx idx) const
    {
        idx = (idx < 0) ? 0 : idx;

        if (idx >= this->numClusters)
        {
            tIDLib::logError("Cluster "+std::to_string(idx)+" does not exist.");
        }
        else
        {
            t_instanceIdx numMembers;
            numMembers = this->clusters[idx].numMembers-1;

            // create local memory
            std::vector<float> listOut(numMembers);

            for (t_instanceIdx i = 0; i < numMembers; ++i)
                listOut[i] = this->clusters[idx].members[i];
            return listOut;
        }
    }

    /**
     * TODO:comment
    */
    std::vector<float> getOrder(t_instanceIdx reference) const
    {
        t_instanceIdx ref;

        // create local memory
        std::vector<tIDLib::t_instance> instancesCopy(this->numInstances);
        std::vector<float> listOut(this->numInstances);

        for (t_instanceIdx i = 0; i < this->numInstances; ++i)
            instancesCopy[i].data.resize(this->instances[i].length);

        if (reference > this->numInstances-1)
            ref = this->numInstances-1;
        else if (reference < 0)
            ref = 0;
        else
            ref = reference;

        // make a local copy of instances so they can be abused
        for (t_instanceIdx i = 0; i < this->numInstances; ++i)
        {
            instancesCopy[i].length = this->instances[i].length;
            instancesCopy[i].clusterMembership = this->instances[i].clusterMembership;
            for (t_instanceIdx j = 0; j < instancesCopy[i].length; ++j)
                instancesCopy[i].data[j] = this->instances[i].data[j];
        }

        for (t_instanceIdx i = 0; i < this->numInstances; ++i)
        {
            float dist;
            float smallest = FLT_MAX;
            t_instanceIdx smallIdx = 0;

            for (t_instanceIdx j = 0; j < this->numInstances; ++j)
            {
                // skip this iteration if this instance slot has already been used.
                if (instancesCopy[j].data[0] == FLT_MAX)
                    continue;

                dist = this->getDist(this->instances[ref], instancesCopy[j]);

                if (dist<smallest)
                {
                    smallest = dist;
                    smallIdx = j;
                }

            }

            listOut[i] = smallIdx; // store the best from this round;

            if (this->relativeOrdering)
                ref = smallIdx; // reorient search to nearest match;

            // set this instance to something huge so it will never be chosen as a good match
            for (t_instanceIdx j = 0; j < this->instances[smallIdx].length; ++j)
                instancesCopy[smallIdx].data[j] = FLT_MAX;
        }

        return listOut;
    }

    /**
     * TODO:comment
    */
    std::vector<float> getMinValues() const
    {
        if (this->normalize)
        {
            // create local memory
            std::vector<float> listOut(this->maxFeatureLength);

            for (t_attributeIdx i = 0; i < this->maxFeatureLength; ++i)
                listOut[i] = this->attributeData[i].normData.min;
            return listOut;
        }
        else
        {
            tIDLib::logError("Feature database not normalized. minimum values not calculated yet.");
            throw std::logic_error("Feature database not normalized. minimum values not calculated yet.");
        }
    }

    /**
     * TODO:comment
    */
    std::vector<float> getMaxValues() const
    {
        if (this->normalize)
        {
            // create local memory
            std::vector<float> listOut(this->maxFeatureLength);

            for (t_attributeIdx i = 0; i < this->maxFeatureLength; ++i)
                listOut[i] = this->attributeData[i].normData.max;

            return listOut;
        }
        else
        {
            tIDLib::logError("Feature database not normalized. maximum values not calculated yet.");
            throw std::logic_error("Feature database not normalized. maximum values not calculated yet.");
        }
    }

    /**
     * TODO:comment
    */
    t_attributeIdx getMinFeatureLength() const
    {
        if (this->numInstances)
        {
            return (this->minFeatureLength);
        }
        else
        {
            tIDLib::logError("No training instances have been loaded.");
            throw std::logic_error("No training instances have been loaded.");
        }
    }

    /**
     * TODO:comment
    */
    t_attributeIdx getMaxFeatureLength() const
    {
        if (this->numInstances)
            return (this->maxFeatureLength);
        else
        {
            tIDLib::logError("No training instances have been loaded.");
            throw std::logic_error("No training instances have been loaded.");
        }
    }

    /**
     * TODO:comment
    */
    std::vector<std::vector<float>> getSimilarityMatrix(t_instanceIdx startInstance, t_instanceIdx finishInstance, bool normalize) const
    {
        if (this->numInstances)
        {
            t_instanceIdx i, j, k, l;

            startInstance = (startInstance<0)?0:startInstance;
            startInstance = (startInstance>=this->numInstances)?this->numInstances-1:startInstance;

            finishInstance = (finishInstance<0)?0:finishInstance;
            finishInstance = (finishInstance>=this->numInstances)?this->numInstances-1:finishInstance;

            if (startInstance > finishInstance)
            {
                t_instanceIdx tmp = finishInstance;
                finishInstance = startInstance;
                startInstance = tmp;
            }

            if (finishInstance - startInstance == 0)
            {
                tIDLib::logError("Bad instance range for similarity matrix");
                std::vector<std::vector<float>> res;
                return res;
            }

            t_instanceIdx numInst = finishInstance-startInstance+1;

            if (numInst)
            {
                float maxDist = -1;

                // create local memory
                // only using t_instance type as a convenient way to make a numInst X numInst matrix
                std::vector<tIDLib::t_instance> distances(numInst);

                for (i = 0; i < numInst; ++i)
                {
                    distances[i].data.resize(numInst);
                    distances[i].length = numInst;
                    for (j = 0; j < distances[i].length; ++j)
                        distances[i].data[j] = 0.0;
                }

                for (i = startInstance, j = 0; i <= finishInstance; ++i, ++j)
                {
                    for (k = startInstance, l = 0; k <= finishInstance; ++k, ++l)
                    {
                        float dist = this->getDist(this->instances[i], this->instances[k]);

                        if (dist>maxDist)
                            maxDist = dist;

                        if (l<distances[i].length)
                            distances[j].data[l] = dist;
                    }
                }

                maxDist = 1.0 / maxDist;



                std::vector<std::vector<float>> res(numInst);
                for (i = startInstance; i < numInst; ++i)
                {
                    std::vector<float> partres(numInst);
                    for (j=0; j<distances[i].length; ++j)
                    {
                        float dist = distances[i].data[j];

                        if (normalize)
                            dist *= maxDist;

                        partres[j] = dist;
                    }
                    res.push_back(partres);
                }

                return res;
            }
            else
            {
                tIDLib::logError("Bad range of instances");
                throw std::logic_error("Bad range of instances");
            }
        }
        else
        {
            tIDLib::logError("No training instances have been loaded.");
            throw std::logic_error("No training instances have been loaded.");
        }
    }

    /**
     * TODO:comment
    */
    std::tuple<std::string,float,t_attributeIdx> getAttributeInfo(t_attributeIdx idx) const
    {
        idx = (idx < 0) ? 0 : idx;

        if (idx >= this->maxFeatureLength)
        {
            std::string errstr = "Attribute "+std::to_string(idx)+" does not exist";
            tIDLib::logError(errstr);
            throw std::logic_error(errstr);
        }
        else
            return std::make_tuple(this->attributeData[idx].name, this->attributeData[idx].weight, this->attributeData[idx].order);
    }

    /* -------- END information functions -------- */

private:

    /**
     * Initialize the parameters of the module.
    */
    void initModule()
    {
        this->maxFeatureLength = 0;
        this->minFeatureLength = INT_MAX;
        this->numClusters = 0;
        this->numInstances = 0;
        this->distMetric = tid::DistanceMetric::euclidean;
        this->kValue = 1;
        this->outputKnnMatches = false;
        this->normalize = false;
        this->relativeOrdering = true;
        this->stutterProtect = false;

        this->concatWrap = true;
        this->prevMatch = UINT_MAX;
        this->maxMatches = 3;
        this->reorientFlag = false;
        this->neighborhood = 0;
        this->searchCenter = 0;
        this->jumpProb = 0.0;
    }

    /* ------------------------- utility functions -------------------------- */

    float attributeMean(t_instanceIdx numRows, t_attributeIdx column, const std::vector<tIDLib::t_instance> &instances, bool normalFlag, const std::vector<tIDLib::t_attributeData> &attributeData) const
    {
        float min, scalar, avg = 0.0;
        for (t_instanceIdx i = 0; i < numRows; ++i)
        {
            // check that this attribute is in range for instance i
            if (column < instances[i].length)
            {
                if (normalFlag)
                {
                    min = attributeData[column].normData.min;
                    scalar = attributeData[column].normData.normScalar;

                    avg += (instances[i].data[column] - min) * scalar;
                }
                else
                    avg += instances[i].data[column];
            }
        }
        return (avg / numRows);
    }

    // this gives the distance between the current inputData and a given instance from this->instances. Same as getDist() below, except more convenient for use in the _id() method because you don't have to fill 2 float buffers in order to use it.
    float getInputDist(t_instanceIdx instanceID) const
    {
        t_attributeIdx vecLen = this->attributeHi - this->attributeLo + 1;
        std::vector<float> vec1Buffer(vecLen);
        std::vector<float> vec2Buffer(vecLen);
        std::vector<float> vecWeights(vecLen);

        float min = 0.0,
              max = 0.0,
              normScalar = 1.0,
              dist = 0.0;

        // extract the right attributes and apply normalization if it's active
        for (t_attributeIdx i = this->attributeLo, j = 0; i <= this->attributeHi; ++i, ++j)
        {
            t_attributeIdx thisAttribute = this->attributeData[i].order;

            if (thisAttribute > this->instances[instanceID].length)
            {
                //TODO: handle with exception or leave return only
                tIDLib::logError("Attribute "+std::to_string(thisAttribute)+" out of range for database instance "+std::to_string(instanceID));
                return(FLT_MAX);
            }

            vecWeights[j] = this->attributeData[thisAttribute].weight;

            if (this->normalize)
            {
                if (this->attributeData[thisAttribute].inputData < this->attributeData[thisAttribute].normData.min)
                    min = this->attributeData[thisAttribute].inputData;
                else
                    min = this->attributeData[thisAttribute].normData.min;

                if (this->attributeData[thisAttribute].inputData > this->attributeData[thisAttribute].normData.max)
                {
                    max = this->attributeData[thisAttribute].inputData;
                    normScalar = 1.0/(max-min);
                }
                else
                {
                    max = this->attributeData[thisAttribute].normData.max;
                    normScalar = this->attributeData[thisAttribute].normData.normScalar;
                }

                vec1Buffer[j] = (this->attributeData[thisAttribute].inputData - min) * normScalar;
                vec2Buffer[j] = (this->instances[instanceID].data[thisAttribute] - min) * normScalar;
            }
            else
            {
                vec1Buffer[j] = this->attributeData[thisAttribute].inputData;
                vec2Buffer[j] = this->instances[instanceID].data[thisAttribute];
            }
        }

        switch(this->distMetric)
        {
            case DistanceMetric::euclidean:
                dist = tIDLib::euclidDist(vecLen, vec1Buffer, vec2Buffer, vecWeights, false);
                break;
            case DistanceMetric::taxi:
                dist = tIDLib::taxiDist(vecLen, vec1Buffer, vec2Buffer, vecWeights);
                break;
            case DistanceMetric::correlation:
                dist = tIDLib::corr(vecLen, vec1Buffer, vec2Buffer);
                // bash to the 0-2 range, then flip sign so that lower is better. this keeps things consistent with other distance metrics.
                dist += 1;
                dist *= -1;
                break;
            default:
                break;
        }

        return(dist);
    }

    // this gives the distance between two vectors stored in the .data part of t_instance arrays.
    float getDist(tIDLib::t_instance instance1, tIDLib::t_instance instance2) const
    {
        t_attributeIdx vecLen = this->attributeHi - this->attributeLo + 1;
        std::vector<float> vec1Buffer(vecLen);
        std::vector<float> vec2Buffer(vecLen);
        std::vector<float> vecWeights(vecLen);

        float min = 0.0,
              max = 0.0,
              normScalar = 1.0,
              dist = 0.0;

        // extract the right attributes and apply normalization if it's active
        for (t_attributeIdx i = this->attributeLo, j = 0; i <= this->attributeHi; ++i, ++j)
        {
            t_attributeIdx thisAttribute;
            thisAttribute = this->attributeData[i].order;

            if (thisAttribute > (instance1.length - 1) || thisAttribute > (instance2.length - 1))
            {
                //TODO: handle this with exeption or only return?
                tIDLib::logError("Attribute "+std::to_string(thisAttribute)+" does not exist for both feature vectors. cannot compute distance.");
                return(FLT_MAX);
            }

            vecWeights[j] = this->attributeData[thisAttribute].weight;

            if (this->normalize)
            {
                // don't need to test if instance1 has attribute values below/above current min/max values in normData, because this function isn't used on input vectors from the wild. It's only used to compare database instances to other database instances, so the normalization terms are valid.
                min = this->attributeData[thisAttribute].normData.min;
                max = this->attributeData[thisAttribute].normData.max;
                normScalar = this->attributeData[thisAttribute].normData.normScalar;

                vec1Buffer[j] = (instance1.data[thisAttribute] - min) * normScalar;
                vec2Buffer[j] = (instance2.data[thisAttribute] - min) * normScalar;
            }
            else
            {
                vec1Buffer[j] = instance1.data[thisAttribute];
                vec2Buffer[j] = instance2.data[thisAttribute];
            }
        }

        switch(this->distMetric)
        {
            case DistanceMetric::euclidean:
                dist = tIDLib::euclidDist(vecLen, vec1Buffer, vec2Buffer, vecWeights, false);
                break;
            case DistanceMetric::taxi:
                dist = tIDLib::taxiDist(vecLen, vec1Buffer, vec2Buffer, vecWeights);
                break;
            case DistanceMetric::correlation:
                dist = tIDLib::corr(vecLen, vec1Buffer, vec2Buffer);
                // bash to the 0-2 range, then flip sign so that lower is better. this keeps things consistent with other distance metrics.
                dist += 1;
                dist *= -1;
                break;
            default:
                break;
        }

        return(dist);
    }

    void attributeDataResize(t_attributeIdx newSize, bool postFlag)
    {
        this->attributeData.resize(newSize);
        this->maxFeatureLength = newSize;

        if (this->maxFeatureLength<this->minFeatureLength)
            this->minFeatureLength = this->maxFeatureLength;

        // initialize attributeData
        for (t_attributeIdx i = 0; i < this->maxFeatureLength; ++i)
        {
            this->attributeData[i].inputData = 0.0;
            this->attributeData[i].order = i;
            this->attributeData[i].weight = 1.0;
            this->attributeData[i].name = "NULL";
        }

        this->normalize = false;
        this->attributeLo = 0;
        this->attributeHi = this->maxFeatureLength - 1;

        if(postFlag)
        {
            std::string res = "";
            res += ("\nmax feature length: "+std::to_string(this->maxFeatureLength));
            res += ("\nmin feature length: "+std::to_string(this->minFeatureLength));
            res += "\nfeature attribute normalization OFF.";
            res += "\nattribute weights initialized";
            res += "\nattribute order initialized";
            res += ("\nattribute range: "+std::to_string(this->attributeLo)+" through "+std::to_string(this->attributeHi));
            tIDLib::logInfo(res);
        }

    }

    /* ------------------------ END utility functions ----------------------- */

    //==========================================================================

    std::vector<tIDLib::t_instance> instances;
    std::vector<tIDLib::t_cluster> clusters;
    std::vector<tIDLib::t_attributeData> attributeData;
    t_attributeIdx maxFeatureLength;
    t_attributeIdx minFeatureLength;
    t_instanceIdx numInstances;
    t_instanceIdx numClusters;
    DistanceMetric distMetric;
    t_instanceIdx kValue;
    bool outputKnnMatches;
    bool normalize;
    bool relativeOrdering;

    bool concatWrap;
    bool reorientFlag;
    t_instanceIdx neighborhood;
    t_instanceIdx searchCenter;
    t_instanceIdx prevMatch;
    t_instanceIdx maxMatches;
    bool stutterProtect;
    float jumpProb;

    t_attributeIdx attributeLo;
    t_attributeIdx attributeHi;
};

} // namespace tid


/* LESS RELEVANT METHODS, YET TO PORT COMPLETELY */
/*

void timbreID_ARFF(t_symbol *s, int argc, t_atom *argv)
{
    FILE *filePtr;
    t_instanceIdx i, j, attRangeLow, attRangeHi, argCount;
    float *featurePtr;
    t_symbol *filenameSymbol, *relationSymbol, *attSymbol;
    char *filename, *relation, *attName, filenameBuf[MAXPDSTRING];

    attRangeLow = 0;
    attRangeHi = 0;
    attSymbol = 0;
    attName = 0;

    argCount = (argc<0)?0:argc;

    filenameSymbol = atom_getsymbol(argv);
    filename = filenameSymbol->s_name;

    canvas_makefilename(this->canvasTODO_mayberemove, filename, filenameBuf, MAXPDSTRING);

    filePtr = fopen(filenameBuf, "w");

    if (!filePtr)
    {
        tIDLib::logError("Failed to create %s", filenameBuf);
        return;
    }

    if (argCount>1)
    {
        relationSymbol = atom_getsymbol(argv+1);
        relation = relationSymbol->s_name;
    }
    else
        relation = "relation";

    fprintf(filePtr, "@RELATION %s\n\n\n", relation);

    if (argCount>2)
    {
        for (i=2; i<argCount; ++i)
        {

            switch((i-2)%3)
            {
                case 0:
                    attRangeLow = atom_getfloat(argv+i);
                    break;
                case 1:
                    attRangeHi = atom_getfloat(argv+i);
                    break;
                case 2:
                    attSymbol = atom_getsymbol(argv+i);
                    attName = attSymbol->s_name;

                    for (j=0; j<=attRangeHi-attRangeLow; ++j)
                        fprintf(filePtr, "@ATTRIBUTE %s-%i NUMERIC\n", attName, j);
                    break;
                default:
                    break;
            }
        }

        // BUG: this was causing a crash because this->maxFeatureLength and attRangeHi are unsigned integers, so we won't get a negative number as expected when there are indeed enough arguments. This came up with version 0.7 because of typedefs - no longer using int. Quick fix is to typecast back to int during the arithmetic

        // in case the argument list was incomplete
        if ((this->maxFeatureLength-1) > attRangeHi)
        {
            for (i=attRangeHi+1, j=0; i<this->maxFeatureLength; ++i, ++j)
                fprintf(filePtr, "@ATTRIBUTE undefined-attribute-%i NUMERIC\n", j);
        }
    }
    else
    {
        for (i=0; i<this->maxFeatureLength; ++i)
            fprintf(filePtr, "@ATTRIBUTE undefined-attribute-%i NUMERIC\n", i);
    }

    fprintf(filePtr, "\n\n");
    fprintf(filePtr, "@DATA\n\n");

    for (i=0; i<this->numInstances; ++i)
    {
        featurePtr = this->instances[i].data;

        for (j=0; j<this->instances[i].length-1; ++j)
            fprintf(filePtr, "%0.20f, ", *featurePtr++);

        // last attribute shouldn't be followed by a comma and space
        fprintf(filePtr, "%0.20f", *featurePtr++);

        fprintf(filePtr, "\n");
    }

    tIDLib::logInfo("Wrote %i non-normalized instances to %s.", this->numInstances, filenameBuf);

    fclose(filePtr);
}


void timbreID_MATLAB(t_symbol *file_symbol, t_symbol *var_symbol)
{
    FILE *filePtr;
    t_instanceIdx i, featuresWritten;
    float *featurePtr;
    char filenameBuf[MAXPDSTRING];

    canvas_makefilename(this->canvasTODO_mayberemove, file_symbol->s_name, filenameBuf, MAXPDSTRING);

    filePtr = fopen(filenameBuf, "w");

    if (!filePtr)
    {
        tIDLib::logError("Failed to create %s", filenameBuf);
        return;
    }

    if (this->maxFeatureLength != this->minFeatureLength)
    {
        tIDLib::logError("Database instances must have uniform length for MATLAB matrix formatting. failed to create %s", filenameBuf);
        fclose(filePtr);
        return;
    }

    fprintf(filePtr, "%% name: %s\n", var_symbol->s_name);
    fprintf(filePtr, "%% type: matrix\n");
    fprintf(filePtr, "%% rows: %i\n", this->numInstances);
    fprintf(filePtr, "%% columns: %i\n\n", this->maxFeatureLength);

    for (i=0; i<this->numInstances; ++i)
    {
        featurePtr = this->instances[i].data;

        featuresWritten=0; // to keep track of each instances no. of features written.

        while (1)
        {
            if (featuresWritten++ == (this->instances[i].length-1))
            {
                fprintf(filePtr, "%0.20f", *featurePtr++);
                break;
            }
            else
                fprintf(filePtr, "%0.20f, ", *featurePtr++);
        }

        fprintf(filePtr, "\n");
    }

    tIDLib::logInfo("Wrote %i non-normalized instances to %s.", this->numInstances, filenameBuf);

    fclose(filePtr);
}


void timbreID_OCTAVE(t_symbol *file_symbol, t_symbol *var_symbol)
{
    FILE *filePtr;
    t_instanceIdx i, featuresWritten;
    float *featurePtr;
    char filenameBuf[MAXPDSTRING];

    canvas_makefilename(this->canvasTODO_mayberemove, file_symbol->s_name, filenameBuf, MAXPDSTRING);

    filePtr = fopen(filenameBuf, "w");

    if (!filePtr)
    {
        tIDLib::logError("Failed to create %s", filenameBuf);
        return;
    }

    if (this->maxFeatureLength != this->minFeatureLength)
    {
        tIDLib::logError("Database instances must have uniform length for MATLAB matrix formatting. failed to create %s", filenameBuf);
        fclose(filePtr);
        return;
    }

    fprintf(filePtr, "# Created by timbreID version %s\n", TIDVERSION);
    fprintf(filePtr, "# name: %s\n", var_symbol->s_name);
    fprintf(filePtr, "# type: matrix\n");
    fprintf(filePtr, "# rows: %i\n", this->numInstances);
    fprintf(filePtr, "# columns: %i\n", this->maxFeatureLength);

    for (i=0; i<this->numInstances; ++i)
    {
        featurePtr = this->instances[i].data;

        featuresWritten=0; // to keep track of each instances no. of features written.

        while (1)
        {
            if (featuresWritten++ == (this->instances[i].length-1))
            {
                fprintf(filePtr, " %0.20f\n", *featurePtr++);
                break;
            }
            else
                fprintf(filePtr, " %0.20f", *featurePtr++);
        }
    }

    tIDLib::logInfo("Wrote %i non-normalized instances to %s.", this->numInstances, filenameBuf);

    fclose(filePtr);
}


void timbreID_FANN(t_symbol *s, float normRange)
{
    FILE *filePtr;
    t_instanceIdx i, j;
    char filenameBuf[MAXPDSTRING];

    canvas_makefilename(this->canvasTODO_mayberemove, s->s_name, filenameBuf, MAXPDSTRING);

    filePtr = fopen(filenameBuf, "w");

    if (!filePtr)
    {
        tIDLib::logError("Failed to create %s", filenameBuf);
        return;
    }

    // write the header containing the number of instances, input size, output size
    fprintf(filePtr, "%i ", this->numInstances);
    // note that this is minFeatureLength assuming that all features will be of the same length so this->minFeatureLength==this->maxFeatureLength
    fprintf(filePtr, "%i ", this->minFeatureLength);
    fprintf(filePtr, "%i ", this->numClusters);
    fprintf(filePtr, "\n");

    for (i=0; i<this->numInstances; ++i)
    {
        for (j=0; j<this->minFeatureLength; ++j)
        {
            if (this->normalize)
            {
                if (normRange)
                    fprintf(filePtr, "%f ", ((this->instances[i].data[j] - this->attributeData[j].normData.min)*this->attributeData[j].normData.normScalar*2.0)-1.0);
                else
                    fprintf(filePtr, "%f ", (this->instances[i].data[j] - this->attributeData[j].normData.min)*this->attributeData[j].normData.normScalar);
            }
            else
                fprintf(filePtr, "%f ", (this->instances[i].data[j]));
        }

        // carriage return between input data line and label line
        fprintf(filePtr, "\n");

        // TODO: fill with all zeros, except a 1.0 in the slot of the cluster idx
        for (j=0; j<this->numClusters; ++j)
        {
            if (j==this->instances[i].clusterMembership)
                fprintf(filePtr, "%i ", 1);
            else
                fprintf(filePtr, "%i ", 0);
        }

        // carriage return between label line and next line of data
        fprintf(filePtr, "\n");
    }

    tIDLib::logInfo("Wrote %i training instances and labels to %s.", this->numInstances, filenameBuf);

    fclose(filePtr);
}
*/
