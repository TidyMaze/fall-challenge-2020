#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

class State;
class Action;
void send(string out);
void debug(string message);
void decideAction(State &state);

enum ActionType
{
    REST,
    CAST,
    OPPONENT_CAST,
    LEARN,
    BREW
};

const ActionType allActionTypes[] = {REST, CAST, OPPONENT_CAST, LEARN, BREW};

class State
{
public:
    vector<Action> actions;
    int myInventory[4];
    int opponentInventory[4];
    int myScore;
    int opponentScore;
};

class Action
{
public:
    int actionId;
    ActionType actionType;
    int deltas[4];
    int price;
    int tomeIndex;
    int taxCount;
    bool castable;
    bool repeatable;
};

class RestAction : public Action
{
public:
    RestAction()
    {
        actionId = -1;
        actionType = ActionType::REST;
        tomeIndex = -1;
    }
};

ActionType parseActionType(string raw)
{
    if (raw == "REST")
    {
        return ActionType::REST;
    }
    else if (raw == "CAST")
    {
        return ActionType::CAST;
    }
    else if (raw == "OPPONENT_CAST")
    {
        return ActionType::OPPONENT_CAST;
    }
    else if (raw == "LEARN")
    {
        return ActionType::LEARN;
    }
    else if (raw == "BREW")
    {
        return ActionType::BREW;
    }
    else
    {
        throw runtime_error("Unhandled action type " + raw);
    }
}

int main()
{
    int turn = 0;

    while (1)
    {
        int actionCount; // the number of spells and recipes in play
        cin >> actionCount;
        cin.ignore();

        State state;

        for (int i = 0; i < actionCount; i++)
        {
            int actionId;      // the unique ID of this spell or recipe
            string actionType; // in the first league: BREW; later: CAST, OPPONENT_CAST, LEARN, BREW
            int delta0;        // tier-0 ingredient change
            int delta1;        // tier-1 ingredient change
            int delta2;        // tier-2 ingredient change
            int delta3;        // tier-3 ingredient change
            int price;         // the price in rupees if this is a potion
            int tomeIndex;     // in the first two leagues: always 0; later: the index in the tome if this is a tome spell, equal to the read-ahead tax; For brews, this is the value of the current urgency bonus
            int taxCount;      // in the first two leagues: always 0; later: the amount of taxed tier-0 ingredients you gain from learning this spell; For brews, this is how many times you can still gain an urgency bonus
            bool castable;     // in the first league: always 0; later: 1 if this is a castable player spell
            bool repeatable;   // for the first two leagues: always 0; later: 1 if this is a repeatable player spell
            cin >> actionId >> actionType >> delta0 >> delta1 >> delta2 >> delta3 >> price >> tomeIndex >> taxCount >> castable >> repeatable;
            cin.ignore();

            Action action;
            action.actionId = actionId;
            action.actionType = parseActionType(actionType);
            action.deltas[0] = delta0;
            action.deltas[1] = delta1;
            action.deltas[2] = delta2;
            action.deltas[3] = delta3;
            action.price = (action.actionType == ActionType::BREW ? tomeIndex : 0);
            action.tomeIndex = tomeIndex;
            action.taxCount = taxCount;
            action.castable = castable;
            action.repeatable = repeatable;

            state.actions.push_back(action);
        }

        // for (int i = 0; i < actions.size(); i++)
        // {
        // debug("actionId " + to_string(actions[i].actionId));
        // }

        for (int i = 0; i < 2; i++)
        {
            int inv0; // tier-0 ingredients in inventory
            int inv1;
            int inv2;
            int inv3;
            int score; // amount of rupees
            cin >> inv0 >> inv1 >> inv2 >> inv3 >> score;
            cin.ignore();

            if (i == 0)
            {
                state.myInventory[0] = inv0;
                state.myInventory[1] = inv1;
                state.myInventory[2] = inv2;
                state.myInventory[3] = inv3;
                state.myScore = score;
            }
            else
            {
                state.opponentInventory[0] = inv0;
                state.opponentInventory[1] = inv1;
                state.opponentInventory[2] = inv2;
                state.opponentInventory[3] = inv3;
                state.opponentScore = score;
            }
        }

        if (turn < 10)
        {
            Action *freeLearn;

            for (Action &a : state.actions)
            {
                if (a.actionType == ActionType::LEARN)
                {
                    freeLearn = &a;
                    break;
                }
            }

            send("LEARN " + to_string(freeLearn->actionId));
        }
        else
        {
            decideAction(state);
        }
        turn++;
    }
}

bool canBuy(State &state, Action &action)
{
    return true;
}

void getAllValidActions(State &s, vector<Action> &dest)
{
    bool canRest = false;

    for (auto &a : s.actions)
    {
        if (a.actionType == ActionType::CAST && !a.castable)
        {
            canRest = true;
        }

        if (canBuy(s, a))
        {
            dest.push_back(a);
        }
    }

    if (canRest)
    {
        RestAction restAction;
        dest.push_back(restAction);
    }
}

void pruneActions(vector<Action> &actions, State &s)
{
}

int invSum(int arr[4])
{
    int res = 0;
    for (int i = 0; i < 4; i++)
    {
        res += arr[i];
    }
    return res;
}

double scoreSide(int score, int inventory[4])
{
    return score * 1000 + invSum(inventory);
}

double scoreState(State &s)
{
    return scoreSide(s.myScore, s.myInventory) - scoreSide(s.opponentScore, s.opponentInventory);
}

void decideAction(State &state)
{
    int MAX_DEPTH = 5;

    vector<pair<vector<Action>, double>> sequenceOfActionToState;

    auto aux = [&, sequenceOfActionToState] (vector<Action> &history, State &s, int depth) mutable {
        vector<Action> buyable;
        getAllValidActions(s, buyable);
        pruneActions(buyable, s);

        if (!history.empty() && (buyable.empty() || depth == 0))
        {
            pair<vector<Action>, double> p = pair{history, scoreState(s)};
            sequenceOfActionToState.push_back(p);
        } else {
            // TODO
        }
    };

    // TODO
    vector<Action> baseHistory = vector<Action> {};
    aux(baseHistory, state, MAX_DEPTH);
}

void send(string out)
{
    cout << out << endl;
}

void debug(string message)
{
    cerr << message << endl;
}