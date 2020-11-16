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

void send(string out);
void sendWait();
void sendRest();
void sendBrewCast(Action &action);
void playAction(State &s, Action &action, State &newState);
int invSum(int arr[4]);

enum ActionType
{
    REST,
    CAST,
    OPPONENT_CAST,
    LEARN,
    BREW
};

string showActionType(ActionType a);

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
        throw runtime_error("Unhandled raw action type " + raw);
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
    int countInv = invSum(state.myInventory);

    switch (action.actionType)
    {
    case ActionType::OPPONENT_CAST:
        return false;
    case ActionType::LEARN:
        return false;
    case ActionType::CAST:
    {
        int countDeltas = invSum(action.deltas);
        bool enoughSpace = countInv + countDeltas <= 10;
        bool enoughInv = (action.deltas[0] + state.myInventory[0] >= 0) &&
                         (action.deltas[1] + state.myInventory[1] >= 0) &&
                         (action.deltas[2] + state.myInventory[2] >= 0) &&
                         (action.deltas[3] + state.myInventory[3] >= 0);
        return action.castable && enoughInv && enoughSpace;
    }
    case ActionType::BREW:
    {
        int countDeltas = invSum(action.deltas);
        bool enoughSpace = countInv + countDeltas <= 10;
        bool enoughInv = (action.deltas[0] + state.myInventory[0] >= 0) &&
                         (action.deltas[1] + state.myInventory[1] >= 0) &&
                         (action.deltas[2] + state.myInventory[2] >= 0) &&
                         (action.deltas[3] + state.myInventory[3] >= 0);
        return enoughInv && enoughSpace;
    }
    }
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

    function<void(vector<Action> &, State &, int)> aux = [&, sequenceOfActionToState](vector<Action> &history, State &s, int depth) mutable {
        vector<Action> buyable;
        getAllValidActions(s, buyable);
        pruneActions(buyable, s);

        if (!history.empty() && (buyable.empty() || depth == 0))
        {
            pair<vector<Action>, double> p = pair{history, scoreState(s)};
            sequenceOfActionToState.push_back(p);
        }
        else
        {
            vector<tuple<vector<Action>, State, double>> childsStates;

            for (auto &a : buyable)
            {
                State newState = s;
                playAction(s, a, newState);
                vector<Action> newHistory = history;
                newHistory.push_back(a);
                double score = scoreState(newState);
                childsStates.push_back({newHistory, newState, score});
            }

            sort(childsStates.begin(), childsStates.end(), [](tuple<vector<Action>, State, double> a, tuple<vector<Action>, State, double> b) {
                return get<2>(a) > get<2>(b);
            });

            for (int i = 0; i < min(3, (int)childsStates.size()); i++)
            {
                aux(get<0>(childsStates[i]), get<1>(childsStates[i]), depth - 1);
            }
        }
    };

    vector<Action> baseHistory = vector<Action>{};
    aux(baseHistory, state, MAX_DEPTH);

    sort(sequenceOfActionToState.begin(), sequenceOfActionToState.end(), [](pair<vector<Action>, double> a, pair<vector<Action>, double> b) {
        return get<1>(a) > get<1>(b);
    });

    if (!sequenceOfActionToState.empty())
    {
        pair<vector<Action>, double> &pickedSequenceToState = sequenceOfActionToState.at(0);
        if (get<0>(pickedSequenceToState)[0].actionType == ActionType::CAST || get<0>(pickedSequenceToState)[0].actionType == ActionType::BREW)
        {
            sendBrewCast(get<0>(pickedSequenceToState)[0]);
        }
        else if (get<0>(pickedSequenceToState)[0].actionType == ActionType::REST)
        {
            sendRest();
        }
        else
        {
            sendWait();
        }
    }
    else
    {
        sendWait();
    }
}

void addInventoryDiff(int (&inv)[4], int diff[4])
{
    for (int i = 0; i < 4; i++)
    {
        inv[i] += diff[i];
    }
}

void playAction(State &s, Action &action, State &newState)
{
    switch (action.actionType)
    {
    case ActionType::CAST:
        for (Action &e : newState.actions)
        {
            if (e.actionId == action.actionId)
            {
                e.castable = false;
                break;
            }
        }
        addInventoryDiff(newState.myInventory, action.deltas);
        break;
    case ActionType::BREW:
        newState.actions.erase(remove_if(newState.actions.begin(), newState.actions.end(), [&, action](Action &e) {
                                   return e.actionId == action.actionId;
                               }),
                               newState.actions.end());
        addInventoryDiff(newState.myInventory, action.deltas);
        newState.myScore += action.price + action.tomeIndex;
        for (int i = 0; i < newState.actions.size(); i++)
        {
            if (newState.actions[i].actionType == ActionType::BREW)
            {
                if (i == 0)
                {
                    newState.actions[i].tomeIndex = 3;
                }
                else if (i == 1)
                {
                    newState.actions[i].tomeIndex = 1;
                }
                else
                {
                    newState.actions[i].tomeIndex = 0;
                }
            }
        }
        break;
    case ActionType::REST:
        for (Action &a : newState.actions)
        {
            if (a.actionType == ActionType::CAST && !a.castable)
            {
                a.castable = true;
            }
        }
        break;
    default:
        throw runtime_error("Unhandled action type " + showActionType(action.actionType));
    }
}

void send(string out)
{
    cout << out << endl;
}

void sendWait()
{
    send("WAIT");
}

void sendRest()
{
    send("REST");
}

string showActionType(ActionType a)
{
    switch (a)
    {
    case ActionType::BREW:
        return "BREW";
    case ActionType::CAST:
        return "CAST";
    case ActionType::OPPONENT_CAST:
        return "OPPONENT_CAST";
    case ActionType::LEARN:
        return "LEARN";
    case ActionType::REST:
        return "REST";
    }
}

void sendBrewCast(Action &action)
{
    send(showActionType(action.actionType) + " " + to_string(action.actionId));
}

void debug(string message)
{
    cerr << message << endl;
}