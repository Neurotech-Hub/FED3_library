#include "FED3.h"

// Handle block transitions and probability updates
void FED3::updateBanditBlock()
{
    static const int probs[2] = {80, 20}; // Standard probability options

    if (BlockPelletCount >= pelletsToSwitch)
    {
        BlockPelletCount = 0; // Reset block counter

        // Select new probability
        int new_prob = probs[random(0, 2)];

        // If repeats not allowed, ensure new prob is different
        if (!allowBlockRepeat)
        {
            while (new_prob == prob_left)
            {
                new_prob = probs[random(0, 2)];
            }
        }

        // Update probabilities
        prob_left = new_prob;
        prob_right = 100 - prob_left;
    }
}

// Handle Bandit-specific poke behavior
bool FED3::handleBanditPoke(bool isLeftPoke)
{
    int poke_prob = isLeftPoke ? prob_left : prob_right;
    bool success = false;

    // Log the poke
    if (isLeftPoke)
    {
        logLeftPoke();
    }
    else
    {
        logRightPoke();
    }

    delay(1000); // Standard delay after poke

    // Check if reward should be delivered
    if (random(100) < poke_prob)
    {
        ConditionedStimulus();
        Feed();
        BlockPelletCount++; // Increment block counter only on successful trials
        success = true;
    }
    else
    {
        Tone(300, 600); // Error tone
    }

    return success;
}

// Initialize Bandit task settings
void FED3::initBanditTask(int pellets_per_block, bool allow_repeats)
{
    countAllPokes = false; // Only count pokes during active trials
    pelletsToSwitch = pellets_per_block;
    allowBlockRepeat = allow_repeats;
    prob_left = 80; // Initial probabilities
    prob_right = 20;
    BlockPelletCount = 0;
}