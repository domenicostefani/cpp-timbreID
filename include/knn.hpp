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

#include "tIDLib.h"
#include <tuple>

namespace tid   /* TimbreID namespace*/
{

using t_prediction = std::tuple<t_instanceIdx,float,float>;

enum class DistanceMetric
{
    euclidean = 0,
    taxi,
    correlation
};

class KNNclassifier
{
public:

    KNNclassifier()
    {
        this->initModule();
    }

    /* -------- classification methods -------- */

    /**
     * Train the model by adding a single feature vector
     * ! NOTE: Calling this is equivalent to sending data to the first inlet of
     *         Puredata external
    */
    t_instanceIdx trainModel(const std::vector<float>& input)
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

        return instanceIdx;
    }

    /**
     * Classify a single sample (one feature vector)
     * Equivalent to sending data to the second inlet
    */
    std::vector<t_prediction> classifySample(const std::vector<float>& input)
    {
        float dist, bestDist, secondBestDist, confidence;
        t_instanceIdx winningID, topVoteInstances;
        unsigned int topVote;
        t_attributeIdx listLength;

        if (this->numInstances)
        {
            listLength = input.size();
            confidence = 0.0f;

            if (listLength > this->maxFeatureLength)
            {
                rtlogger.logInfo("Input feature list longer than current max feature length of database. input ignored.");
                return {};
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
                    return {std::make_tuple(-1,-1.0f,-1.0f)};

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

            confidence = 1.0f - (bestDist / secondBestDist);

            t_prediction partres = std::make_tuple(winningID,confidence,bestDist);

            std::vector<t_prediction> res;
            res.push_back(partres);

            if (this->outputKnnMatches)
            {
                for (t_instanceIdx i = 0; i < this->kValue; ++i)
                {
                    // suppress reporting of the vote-winning id, because it was already reported above
                    if (this->instances[i].knnInfo.cluster != winningID)
                    {
                        t_prediction partres = std::make_tuple(this->instances[i].knnInfo.cluster,-1,this->instances[i].knnInfo.safeDist);
                        res.push_back(partres);
                    }
                }
            }

            return res;
        }
        else
        {
            rtlogger.logInfo("No training instances have been loaded. cannot perform ID.");
            return {};
        }
    }

    t_prediction worstMatch(const std::vector<float>& input)
    {
        float dist, worstDist;
        t_instanceIdx i, losingID;
        if (this->numInstances)
        {
            if (input.size() > this->maxFeatureLength)
            {
                rtlogger.logInfo("Input feature list too long. input ignored.");
                return std::make_tuple(-1,-1.0f,-1.0f);
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
                    return std::make_tuple(-1,-1.0f,-1.0f);

                if (dist > worstDist)
                {
                    worstDist = dist;
                    losingID = i;
                }
            }

            losingID = this->instances[losingID].clusterMembership;

            t_prediction res = std::make_tuple(losingID,0,worstDist);
            return res;
        }
        else
        {
            rtlogger.logInfo("No training instances have been loaded. cannot perform worst match.");
            return std::make_tuple(-1,-1,-1);
        }
    }

    /* -------- END classification methods -------- */

    /* -------- concatenative synthesis methods -------- */

    /**
     * Concatenative synthesis method
     * See the concatenative.pd example in the original timbreID github
     * repository examples package for a fleshed-out example of how various
     * parameters can be useful.
     * Equivalent to sending data to the third inlet
    */
    t_prediction concatId(const std::vector<float>& input)
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

            halfNeighborhood = this->neighborhood*0.5f;

            if (listLength > this->maxFeatureLength)
            {
                rtlogger.logInfo("Input feature list too long. input ignored.");
                return std::make_tuple(-1,-1.0f,-1.0f);
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
                    return std::make_tuple(-1,-1.0f,-1.0f);

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
                        return std::make_tuple(-1,-1.0f,-1.0f);

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

            return std::make_tuple(bestDist,-1,winningID);
        }
        else
        {
            rtlogger.logInfo("No training instances have been loaded. cannot perform ID.");
            return std::make_tuple(-1,-1,-1);
        }
    }

    /**
     * Turn on/off database search wrapping when using concatenative synthesis
     * (default: ON).
     * If the search_center and neighborhood settings cause the search range to
     * extend below instance 0 or past the last instance in the database,
     * this module will make the search range wrap around to the beginning or
     * end of the database accordingly.
     * With this feature turned off, timbreID will clip the search range and not
     * wrap around.
    */
    void concatSearchWrap(bool flag)
    {
        this->concatWrap = flag;
    }

    /**
     * Restrict the search to a neighborhood of instances around a specific
     * center (see search_center below). With a center of 30 and neighborhood
     * of 7, timbreID will search instances 27-33. If the neighborhood is even
     * the range above search_center will be one greater than that below.
    */
    void concatNeighborhood(t_instanceIdx n)
    {
        n = (n > this->numInstances) ? this->numInstances : n;
        n = (n < 1) ? 1 : n;
        this->neighborhood = n;
    }

    /**
     * Set a probability between 0-1 that search center will be randomly
     * reassigned on each identification request.
    */
    void concatJumpProb(float jp)
    {
        jp = (jp < 0) ? 0 : jp;
        jp = (jp > 1) ? 1 : jp;
        this->jumpProb = jp;
    }

    /**
     * With reorient on, the search_center will be constantly updated as the
     * most recent match.
    */
    void concatReorient(bool r)
    {
        this->reorientFlag = r;
    }

    /**
     * Specify the center instance for searching within a neighborhood.
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
     * The "max_matches" setting can have an impact on how continuous the
     * concatenated audio output sounds. eg. If max_matches==5, timbreID will
     * find the top 5 matches, and check to see how they compare with the grain
     * from the previous match. If one of these 5 is a better match to the
     * previous grain than the current input grain, it will be reported as the
     * best match.
    */
    void concatMaxMatches(t_instanceIdx mm)
    {
        mm = (mm < 1) ? 1 : mm;
        this->maxMatches = (mm > this->numInstances) ? this->numInstances : mm;
    }

    /**
     * With the "stutter_protect" option on, timbreID does not allow the same
     * match to be output twice in a row.
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
        if (k < 1.0f)
        {
            char errtxt[RealTimeLogger::LogEntry::MESSAGE_LENGTH+1] = "K must be greater than one.";
            rtlogger.logInfo(errtxt);
            throw std::invalid_argument(errtxt);
        }

        if (k > this->numInstances)
        {
            char errtxt[RealTimeLogger::LogEntry::MESSAGE_LENGTH+1] = "K must be less than the total number of instances.";
            rtlogger.logInfo(errtxt);
            throw std::invalid_argument(errtxt);
        }

        this->kValue = k;
        rtlogger.logValue("K value (neighbors): ",this->kValue);
    }

    /**
     * Output the distances and instance (or cluster) IDs of the K nearest
     * neighbors in addition to the winning match.
     * In the case of clustered data, this will only output the neighbors
     * that are not members of the winning cluster.
    */
    void setOutputKnnMatches(bool out)
    {
        this->outputKnnMatches = out;

        if (this->outputKnnMatches)
            rtlogger.logInfo("Reporting all KNN matches.");
        else
            rtlogger.logInfo("Reporting best match only.");
    }

    /**
     * Normalize all feature attributes to the 0-1 range.
     * If the range of some attributes is much larger than others, they will
     * have more of an influence on the distance calculation, which may skew
     * things in an undesireable way.
     * Normalization evens the playing field. BUT - it also means that noisy
     * attributes with a small magnitude (like the highest MFCCs) might have
     * undue influence.
     * In the case of mixing spectral centroid and zero crossing rate into a
     * single feature, however, normalization is almost certainly the way to go.
     * Note: if you have thousands rather than hundreds of instances,
     * this calculation will take several seconds. This is something to be
     * performed before any real time classification is going on.
    */
    void normalizeAttributes(bool normalize)
    {
        // create local memory
        std::vector<float> attributeColumn(this->numInstances);

        if (!normalize)
        {
            // initialize normData
            for (t_attributeIdx i = 0; i < this->maxFeatureLength; ++i)
            {
                this->attributeData[i].normData.min = 0.0f;
                this->attributeData[i].normData.max = 0.0f;
                this->attributeData[i].normData.normScalar = 1.0f;
            }

            this->normalize = false;
            rtlogger.logInfo("Eature attribute normalization OFF.");
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
                            char message[tid::RealTimeLogger::LogEntry::MESSAGE_LENGTH+1];
                            snprintf(message,sizeof(message),"Attribute %d out of range for database instance %d. Aborting normalization",(int)j,(int)i);
                            rtlogger.logInfo(message);
                            this->normalize = false;
                            rtlogger.logInfo("Feature attribute normalization OFF.");
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
                        this->attributeData[j].normData.min = 0.0f;
                        this->attributeData[j].normData.normScalar = 1.0f;
                    }
                    else
                        this->attributeData[j].normData.normScalar = 1.0f/(this->attributeData[j].normData.max - this->attributeData[j].normData.min);

                }

                this->normalize = true;
                rtlogger.logInfo("Feature attribute normalization ON.");
            }
            else
                rtlogger.logInfo("No training instances have been loaded. cannot calculate normalization terms.");
        }
    }

    /**
     * Cluster manually a single instances subset
     * This is an alternative to auto-clustering which might be useful when
     * having to cluster things in the middle of a performance and needing to be
     * able to do it entirely within this module.
     * Call this method for each cluster you want to create.
     * Argument indexing is always starting from 0.
     * Notice that you can't cluster non-neighboring instances at present.
     * Also note that this module does not check to see if you actually end up
     * defining X clusters if you've called a few times this method indicating
     * that there will be X clusters.
     * Crashes are entirely possible if you don't keep track of this properly.
     *
     * @param numClusters  total number of clusters
     * @param clusterIdx   this cluster index
     * @param low          lower instance index
     * @param hi           upper instance index
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
            rtlogger.logInfo("Instances out of range. clustering failed.");
            return;
        }

        if (clusterIdx >= numClusters)
        {
            char message[tid::RealTimeLogger::LogEntry::MESSAGE_LENGTH+1];
            snprintf(message,sizeof(message),"Cluster index %u out of range for given number of clusters %u. clustering failed.",clusterIdx,numClusters);
            rtlogger.logInfo(message);
            return;
        }

        if (this->numInstances < numClusters)
        {
            rtlogger.logInfo("Not enough instances to cluster. clustering failed.");
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

            char message[tid::RealTimeLogger::LogEntry::MESSAGE_LENGTH+1];
            snprintf(message,sizeof(message),"Cluster %u contains instances %u through %u.",clusterIdx,low,hi);
            rtlogger.logInfo("message");
        }
    }

    /**
     * Perform hierarchical clustering to find a given number of clusters
     *
     * @param numClusters number of clusters to find
    */
    std::vector<float> autoCluster(t_instanceIdx numClusters)
    {
        std::vector<float> listOut;
        if (this->numInstances < numClusters)
            rtlogger.logInfo("Not enough instances to cluster.");
        else if (this->numClusters != this->numInstances)
            rtlogger.logInfo("Instances already clustered. uncluster first.");
        else if (numClusters==0)
            rtlogger.logInfo("Cannot create 0 clusters.");
        else
        {
            t_instanceIdx i, j, k, numInstances, numInstancesM1, numPairs, numClusterMembers1, numClusterMembers2, numClusterMembersSum, clusterCount;

            std::vector<t_instanceIdx> minDistIdx;
            std::vector<tIDLib::t_instance> clusterData;
            std::vector<float> pairDists;
            float minDist, numClusterMembers1_recip;

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
                    numClusterMembers1_recip = 1.0f/(float)numClusterMembers1;
                else
                    numClusterMembers1_recip = 1.0f;

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

            rtlogger.logInfo("Instances clustered.");
        }
        return listOut;
    }

    /**
     * Uncluster data
     * When having already clustered data, go back to reporting raw instance
     * indexes.
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

        rtlogger.logInfo("Instances unclustered.");
    }

    /**
     * Order attributes by variance, so that only the most relevant attributes
     * can be used to calculate the distance measure.
     * For instance, after ordering a 47-component BFCC vector by variance, you
     * may only want to compare the first 10 attributes, since those will be the
     * ones with the most variance.Specify this range using the
     * setAttributeRange method.
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
                    meanCentered[i].data[j] = 0.0f;

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
                float max = 0.0f;
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

            rtlogger.logInfo("Attributes ordered by variance.");
        }
        else
        {
            rtlogger.logInfo("No instances for variance computation.");
            throw std::logic_error("No instances for variance computation.");
        }
    }

    /**
     * Set type of ordering (absolute or relative)
     * There are two options for timbre order requests: relative and absolute.
     * With absolute ordering, instances are reported in order of increasing
     * distance from the reference instance given. With relative ordering, the
     * reference instance changes at every step. First, the nearest instance to
     * the given reference instance is found, then the nearest instance to that
     * instance is found, and so on. This creates smoother timbre transitions
     * throughout the entire instance ordering, and is the default setting
    */
    void setRelativeOrdering(bool rel)
    {
        this->relativeOrdering = rel;

        if (this->relativeOrdering)
            rtlogger.logInfo("Relative ordering ON.");
        else
            rtlogger.logInfo("Relative ordering OFF.");
    }

    /**
     * Set the preferred distance metric
    */
    void setDistMetric(DistanceMetric d)
    {
        this->distMetric = d;

        switch(this->distMetric)
        {
            case DistanceMetric::euclidean:
                rtlogger.logInfo("Distance metric: EUCLIDEAN.");
                break;
            case DistanceMetric::taxi:
                rtlogger.logInfo("Distance metric: MANHATTAN (taxicab distance).");
                break;
            case DistanceMetric::correlation:
                rtlogger.logInfo("Distance metric: PEARSON CORRELATION COEFF.");
                break;
            default:
                break;
        }
    }

    /**
     * Specify a list of weights.
     * Suppose having a feature vector composed of spectral centroid and
     * spectral flux, and wanting the latter feature to have half as much impact
     * as the former during distance calculation.
     * In this case the method must be called like: setWeights({1.0, 0.5})
    */
    void setWeights(std::vector<float> weights)
    {
        t_attributeIdx i;
        t_attributeIdx listLength = weights.size();

        if (listLength > this->maxFeatureLength)
        {
            rtlogger.logInfo("Weights list longer than current feature length. ignoring excess weights");
            listLength = this->maxFeatureLength;
        }

        for (i=0; i<listLength; ++i)
            this->attributeData[i].weight = weights[i];

        // if only the first few of a long feature vector are specified, fill in the rest with 1.0
        for (; i<this->maxFeatureLength; ++i)
            this->attributeData[i].weight = 1.0f;
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
            rtlogger.logInfo("Attribute list longer than current max feature length. ignoring excess attributes");
            listLength = this->maxFeatureLength;
        }

        for (i=0; i<listLength; ++i)
            this->attributeData[i].name = names[i];

        rtlogger.logInfo("Attribute names received.");
    }

    /**
     * Manually order the attributes.
     * In conjunction with attribute_range, this is
     * useful for exporting specific subsets of attributes, or doing distance
     * calculation based on subsets of attributes
    */
    void setAttributeOrder(std::vector<t_attributeIdx> order)
    {
        t_attributeIdx i, j, listLength;

        listLength = order.size();

        if (listLength > this->maxFeatureLength)
        {
            rtlogger.logInfo("Attribute list longer than current max feature length. ignoring excess attributes");
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

                rtlogger.logValue("Attribute",this->attributeData[i].order,"out of range. attribute order initialized.");
            }
        }

        // fill any remainder with attribute 0
        for (; i<this->maxFeatureLength; ++i)
            this->attributeData[i].order = 0;

        rtlogger.logInfo("Attributes re-ordered.");
    }

    /**
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

        char message[tid::RealTimeLogger::LogEntry::MESSAGE_LENGTH+1];
        snprintf(message,sizeof(message),"Attribute range: %u through %u.",attributeLo,attributeHi);
        rtlogger.logInfo(message);
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

        rtlogger.logInfo("Attribute order initialized.");
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

        rtlogger.logInfo("All instances cleared.");
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
    bool writeData(std::string filename)
    {
        FILE *filePtr;

        // make a buffer for the header data, which is the number of instances followed by the length of each instance
        std::vector<t_instanceIdx> header(this->numInstances+1);

        filePtr = fopen(filename.c_str(), "wb");

        if (!filePtr)
        {
            rtlogger.logInfo("Failed to create ",filename.c_str());
            return false;
        }

        // write the header information, which is the number of instances, and then the length of each instance
        header[0] = this->numInstances;
        for (t_instanceIdx i = 0; i < this->numInstances; ++i)
            header[i+1] = this->instances[i].length;

        fwrite(&(header[0]), sizeof(t_instanceIdx), this->numInstances+1, filePtr);

        // write the instance data
        for (t_instanceIdx i = 0; i < this->numInstances; ++i)
            fwrite(&(this->instances[i].data[0]), sizeof(float), this->instances[i].length, filePtr);

        char message[tid::RealTimeLogger::LogEntry::MESSAGE_LENGTH+1];
        snprintf(message,sizeof(message),"Wrote %u non-normalized instances to %s.",this->numInstances,filename.c_str());
        rtlogger.logInfo(message);

        fclose(filePtr);
        return true;
    }

    /*
     * TimbreID's default binary file format is .timid. This will write and read
     * much faster than the text format, so it's the best choice for large databases
     * (i.e. 1000s of instances)
     * eg. writeData("./data/feature-db.timid")
    */
    bool readData(std::string filename)
    {
        FILE *filePtr;
        t_instanceIdx i;

        filePtr = fopen(filename.c_str(), "rb");

        if (!filePtr)
        {
            rtlogger.logInfo("Failed to open ",filename.c_str());
            return false;
        }

        t_instanceIdx maxLength = 0;
        t_instanceIdx minLength = INT_MAX;

        // erase old instances & clusters and resize to 0. this also does a sub-call to this->attributeDataResize()
        this->clearAll();

        // first item in the header is the number of instances
        fread(&this->numInstances, sizeof(t_instanceIdx), 1, filePtr);

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

        char message[tid::RealTimeLogger::LogEntry::MESSAGE_LENGTH+1];
        snprintf(message,sizeof(message),"Read %u instances from %s.",this->numInstances,filename.c_str());
        rtlogger.logInfo(message);

        fclose(filePtr);
        return true;
    }

    /**
     * When needing to look at the feature database values, data can be exported
     * in the text file format using this method
    */
    bool writeTextData(std::string filename)
    {
        FILE *filePtr;
        t_instanceIdx i;
        t_attributeIdx j;

        filePtr = fopen(filename.c_str(), "w");

        if (!filePtr)
        {
            rtlogger.logInfo("Failed to create ",filename.c_str());
            return false;
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

        char message[tid::RealTimeLogger::LogEntry::MESSAGE_LENGTH+1];
        snprintf(message,sizeof(message),"Wrote %u instances to %s.",this->numInstances,filename.c_str());
        rtlogger.logInfo(message);

        fclose(filePtr);
        return true;
    }

    /**
     * Used to read text file format
    */
    bool readTextData(std::string filename)
    {
        FILE *filePtr;
        t_instanceIdx i, j, numInstances, stringLength, numSpaces;
        char textLine[tIDLib::MAXTIDTEXTSTRING];

        filePtr = fopen(filename.c_str(), "r");

        if (!filePtr)
        {
            rtlogger.logInfo("Failed to open ",filename.c_str());
            return false;
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

        char message[tid::RealTimeLogger::LogEntry::MESSAGE_LENGTH+1];
        snprintf(message,sizeof(message),"Read %u instances from %s.",this->numInstances,filename.c_str());
        rtlogger.logInfo(message);

        fclose(filePtr);
        return true;
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
            rtlogger.logInfo("Failed to create ",filename.c_str());
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

        char message[tid::RealTimeLogger::LogEntry::MESSAGE_LENGTH+1];
        snprintf(message,sizeof(message),"Wrote %u clusters to %s.",this->numClusters,filename.c_str());
        rtlogger.logInfo(message);

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
            rtlogger.logInfo("Failed to open ",filename.c_str());
            return;
        }

        // read header indicating number of clusters
        fread(&numClusters, sizeof(t_instanceIdx), 1, filePtr);

        if (numClusters>this->numInstances)
        {
            rtlogger.logInfo(filename.c_str()," contains more clusters than current number of database instances. read failed.");
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

        char message[tid::RealTimeLogger::LogEntry::MESSAGE_LENGTH+1];
        snprintf(message,sizeof(message),"Read %u clusters from %s.",this->numClusters,filename.c_str());
        rtlogger.logInfo(message);

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
            rtlogger.logInfo("Failed to create ",filename.c_str());
            return;
        }

        for (i=0; i<this->numClusters; ++i)
        {
            for (j=0; j<this->clusters[i].numMembers-2; ++j)
                fprintf(filePtr, "%i ", this->clusters[i].members[j]);

            // no space for the final instance given on the line for cluster i
            fprintf(filePtr, "%i", this->clusters[i].members[this->clusters[i].numMembers-1]);

            // newline to end the list of instances for cluster i
            fprintf(filePtr, "\n");
        }

        char message[tid::RealTimeLogger::LogEntry::MESSAGE_LENGTH+1];
        snprintf(message,sizeof(message),"Wrote %u clusters to %s.",this->numClusters,filename.c_str());
        rtlogger.logInfo(message);
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
            rtlogger.logInfo("Failed to open ",filename.c_str());
            return;
        }

        // count lines in text file to determine numClusters
        numClusters = 0;
        while (fgets(textLine, tIDLib::MAXTIDTEXTSTRING, filePtr))
            numClusters++;

        if (numClusters>this->numInstances)
        {
            rtlogger.logInfo(filename.c_str()," contains more clusters than current number of database instances. read failed.");
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

        char message[tid::RealTimeLogger::LogEntry::MESSAGE_LENGTH+1];
        snprintf(message,sizeof(message),"Read %u clusters from %s.",this->numClusters,filename.c_str());
        rtlogger.logInfo(message);

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
        return res;
    }

    /** Information functions (Fourth outlet data in Puredata Version) -------*/

    /**
     * Returns the number of instances in the dataset
    */
    t_instanceIdx getNumInstances() const
    {
        return this->numInstances;
    }

    /**
     * Returns the feature vector for the specified instance
    */
    std::vector<float> getFeatureVec(t_instanceIdx idx)
    {
        idx = (idx < 0) ? 0 : idx;

        // create local memory
        std::vector<float> listOut;

        if (idx > this->numInstances-1)
        {
            rtlogger.logValue("Instance",idx,"does not exist.");
            throw std::logic_error("Instance"+std::to_string(idx)+"does not exist.");
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
     * Returns the cluster to which the specified instance belongs.
    */
    t_instanceIdx getClusterMembership(t_instanceIdx idx)
    {
        if (idx >= this->numInstances)
        {
            rtlogger.logValue("Instance",(idx),"does not exist.");
            throw std::logic_error("Instance "+std::to_string(idx)+" does not exist.");
        }
        else
        {
            return this->instances[idx].clusterMembership;
        }
    }

    /**
     * Returns the list of the datset instances ordered by cluster
     * (Notice the difference of singular vs. plural - clustersList vs.
     * clusterList)
    */
    std::vector<float> getClustersList()
    {
        // create local memory
        std::vector<float> listOut(this->numInstances);

        for (t_instanceIdx i = 0, k = 0; i < this->numClusters; ++i)
            for (t_instanceIdx j = 0; j < (this->clusters[i].numMembers-1); ++j, ++k) // -1 because it's terminated by UINT_MAX
                listOut[i] = this->clusters[i].members[j];
        return listOut;
    }

    /**
     * Returns the member list of only one specific cluster.
     * (Notice the difference of singular vs. plural - clustersList vs.
     * clusterList)
    */
    std::vector<float> clusterList(t_instanceIdx idx)
    {
        idx = (idx < 0) ? 0 : idx;

        if (idx >= this->numClusters)
        {
            rtlogger.logValue("Cluster",idx,"does not exist.");
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
     * Try an order starting from a particular instrument/cluster
    */
    std::vector<float> setOrder(t_instanceIdx reference)
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
     * Spit out a list of the min values for all attributes in the feature
     * database.
    */
    std::vector<float> getMinValues()
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
            rtlogger.logInfo("Feature database not normalized. minimum values not calculated yet.");
            throw std::logic_error("Feature database not normalized. minimum values not calculated yet.");
        }
    }

    /**
     * Spit out a list of the max values for all attributes in the feature
     * database.
    */
    std::vector<float> getMaxValues()
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
            rtlogger.logInfo("Feature database not normalized. maximum values not calculated yet.");
            throw std::logic_error("Feature database not normalized. maximum values not calculated yet.");
        }
    }

    /**
     * Query the minimum instance length in the database
    */
    t_attributeIdx getMinFeatureLength()
    {
        if (this->numInstances)
        {
            return (this->minFeatureLength);
        }
        else
        {
            rtlogger.logInfo("No training instances have been loaded.");
            throw std::logic_error("No training instances have been loaded.");
        }
    }

    /**
     * Query the maximum instance length in the database
    */
    t_attributeIdx getMaxFeatureLength()
    {
        if (this->numInstances)
            return (this->maxFeatureLength);
        else
        {
            rtlogger.logInfo("No training instances have been loaded.");
            throw std::logic_error("No training instances have been loaded.");
        }
    }

    /**
     * Generate a similarity matrix.
     * Arguments are starting and ending instance indices, and a normalization
     * flag. The data is sent out as N lists with N elements in each (where N is
     * the number of instances in the requested range).
    */
    std::vector<std::vector<float>> getSimilarityMatrix(t_instanceIdx startInstance, t_instanceIdx finishInstance, bool normalize)
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
                rtlogger.logInfo("Bad instance range for similarity matrix");
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
                        distances[i].data[j] = 0.0f;
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

                maxDist = 1.0f / maxDist;



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
                rtlogger.logInfo("Bad range of instances");
                throw std::logic_error("Bad range of instances");
            }
        }
        else
        {
            rtlogger.logInfo("No training instances have been loaded.");
            throw std::logic_error("No training instances have been loaded.");
        }
    }

    /**
     * Returns a tuple containing the name, the weight and the order of the
     * specified attribute
    */
    std::tuple<std::string,float,t_attributeIdx> getAttributeInfo(t_attributeIdx idx)
    {
        idx = (idx < 0) ? 0 : idx;

        if (idx >= this->maxFeatureLength)
        {
            rtlogger.logValue("Attribute",idx,"does not exist");
            throw std::logic_error("Attribute "+std::to_string(idx)+" does not exist");
        }
        else
            return std::make_tuple(this->attributeData[idx].name, this->attributeData[idx].weight, this->attributeData[idx].order);
    }

    /**
     * Return a pointer to the tid::RealTimeLogger used, so that log Messages
     * can be consumed outside of the real-time thread execution
    */
    tid::RealTimeLogger* getLoggerPtr()
    {
        return &rtlogger;
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
        this->distMetric = DistanceMetric::euclidean;
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
        this->jumpProb = 0.0f;
    }

    /* ------------------------- utility functions -------------------------- */

    float attributeMean(t_instanceIdx numRows, t_attributeIdx column, const std::vector<tIDLib::t_instance> &instances, bool normalFlag, const std::vector<tIDLib::t_attributeData> &attributeData) const
    {
        float min, scalar, avg = 0.0f;
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
    float getInputDist(t_instanceIdx instanceID)
    {
        t_attributeIdx vecLen = this->attributeHi - this->attributeLo + 1;
        std::vector<float> vec1Buffer(vecLen);
        std::vector<float> vec2Buffer(vecLen);
        std::vector<float> vecWeights(vecLen);

        float min = 0.0f,
              max = 0.0f,
              normScalar = 1.0f,
              dist = 0.0f;

        // extract the right attributes and apply normalization if it's active
        for (t_attributeIdx i = this->attributeLo, j = 0; i <= this->attributeHi; ++i, ++j)
        {
            t_attributeIdx thisAttribute = this->attributeData[i].order;

            if (thisAttribute > this->instances[instanceID].length)
            {
                char message[tid::RealTimeLogger::LogEntry::MESSAGE_LENGTH+1];
                snprintf(message,sizeof(message),"Attribute %d out of range for database instance %d.",(int)thisAttribute,(int)instanceID);
                rtlogger.logInfo(message);
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
                    normScalar = 1.0f/(max-min);
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
    float getDist(tIDLib::t_instance instance1, tIDLib::t_instance instance2)
    {
        t_attributeIdx vecLen = this->attributeHi - this->attributeLo + 1;
        std::vector<float> vec1Buffer(vecLen);
        std::vector<float> vec2Buffer(vecLen);
        std::vector<float> vecWeights(vecLen);

        float normScalar = 1.0f,
              dist = 0.0f;

        // extract the right attributes and apply normalization if it's active
        for (t_attributeIdx i = this->attributeLo, j = 0; i <= this->attributeHi; ++i, ++j)
        {
            t_attributeIdx thisAttribute;
            thisAttribute = this->attributeData[i].order;

            if (thisAttribute > (instance1.length - 1) || thisAttribute > (instance2.length - 1))
            {
                rtlogger.logValue("Attribute",thisAttribute,"does not exist for both feature vectors. cannot compute distance.");
                return(FLT_MAX);
            }

            vecWeights[j] = this->attributeData[thisAttribute].weight;

            if (this->normalize)
            {
                // don't need to test if instance1 has attribute values below/above current min/max values in normData, because this function isn't used on input vectors from the wild. It's only used to compare database instances to other database instances, so the normalization terms are valid.
                normScalar = this->attributeData[thisAttribute].normData.normScalar;

                vec1Buffer[j] = (instance1.data[thisAttribute] - this->attributeData[thisAttribute].normData.min) * normScalar;
                vec2Buffer[j] = (instance2.data[thisAttribute] - this->attributeData[thisAttribute].normData.min) * normScalar;
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
            this->attributeData[i].inputData = 0.0f;
            this->attributeData[i].order = i;
            this->attributeData[i].weight = 1.0f;
            this->attributeData[i].name = "NULL";
        }

        this->normalize = false;
        this->attributeLo = 0;
        this->attributeHi = this->maxFeatureLength - 1;

        if(postFlag)
        {
            rtlogger.logValue("max feature length:",this->maxFeatureLength);
            rtlogger.logValue("min feature length:",this->minFeatureLength);
            rtlogger.logInfo("\nfeature attribute normalization OFF.");
            rtlogger.logInfo("\nattribute weights initialized");
            rtlogger.logInfo("\nattribute order initialized");

            char message[tid::RealTimeLogger::LogEntry::MESSAGE_LENGTH+1];
            snprintf(message,sizeof(message),"attribute range: %u through %u",this->attributeLo,this->attributeHi);
            rtlogger.logInfo(message);
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

    tid::RealTimeLogger rtlogger { "knn (~timbreId)" };
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

    canvas_makefilename(this->canvas mayberemove, filename, filenameBuf, MAXPDSTRING);

    filePtr = fopen(filenameBuf, "w");

    if (!filePtr)
    {
        rtlogger.logInfo("Failed to create ",filename.c_str());
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

    rtlogger.logInfo("Wrote %i non-normalized instances to %s.", this->numInstances, filenameBuf);

    fclose(filePtr);
}


void timbreID_MATLAB(t_symbol *file_symbol, t_symbol *var_symbol)
{
    FILE *filePtr;
    t_instanceIdx i, featuresWritten;
    float *featurePtr;
    char filenameBuf[MAXPDSTRING];

    canvas_makefilename(this->canvas maybe remove, file_symbol->s_name, filenameBuf, MAXPDSTRING);

    filePtr = fopen(filenameBuf, "w");

    if (!filePtr)
    {
        rtlogger.logInfo("Failed to create ",filename.c_str());
        return;
    }

    if (this->maxFeatureLength != this->minFeatureLength)
    {
        rtlogger.logInfo("Database instances must have uniform length for MATLAB matrix formatting. failed to create %s", filenameBuf);
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

    rtlogger.logInfo("Wrote %i non-normalized instances to %s.", this->numInstances, filenameBuf);

    fclose(filePtr);
}


void timbreID_OCTAVE(t_symbol *file_symbol, t_symbol *var_symbol)
{
    FILE *filePtr;
    t_instanceIdx i, featuresWritten;
    float *featurePtr;
    char filenameBuf[MAXPDSTRING];

    canvas_makefilename(this->canvas maybe remove, file_symbol->s_name, filenameBuf, MAXPDSTRING);

    filePtr = fopen(filenameBuf, "w");

    if (!filePtr)
    {
        rtlogger.logInfo("Failed to create %s", filenameBuf);
        return;
    }

    if (this->maxFeatureLength != this->minFeatureLength)
    {
        rtlogger.logInfo("Database instances must have uniform length for MATLAB matrix formatting. failed to create %s", filenameBuf);
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

    rtlogger.logInfo("Wrote %i non-normalized instances to %s.", this->numInstances, filenameBuf);

    fclose(filePtr);
}


void timbreID_FANN(t_symbol *s, float normRange)
{
    FILE *filePtr;
    t_instanceIdx i, j;
    char filenameBuf[MAXPDSTRING];

    canvas_makefilename(this->canvas maybe remove, s->s_name, filenameBuf, MAXPDSTRING);

    filePtr = fopen(filenameBuf, "w");

    if (!filePtr)
    {
        rtlogger.logInfo("Failed to create %s", filenameBuf);
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

    rtlogger.logInfo("Wrote %i training instances and labels to %s.", this->numInstances, filenameBuf);

    fclose(filePtr);
}
*/
