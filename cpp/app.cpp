#pragma GCC optimize("O3")
#pragma GCC optimize("inline")
#pragma GCC optimize("omit-frame-pointer")
#pragma GCC optimize("unroll-loops") //Optimization flags

#pragma GCC optimize("Ofast")

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>

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

const int MAX_CHILDS_PER_NODE = 100;

const int TIMEOUT_MS = 40;

uint64_t start;

int countChildNodes;
int sumChildNodes;
int countVisitedNodes;

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
    int repeat = 1;
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

uint64_t millis()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

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

        start = millis();

        countChildNodes = 0;
        sumChildNodes = 0;
        countVisitedNodes = 0;

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
        bool enoughSpace = countInv + action.repeat * countDeltas <= 10;
        bool enoughInv = (action.repeat * action.deltas[0] + state.myInventory[0] >= 0) &&
                         (action.repeat * action.deltas[1] + state.myInventory[1] >= 0) &&
                         (action.repeat * action.deltas[2] + state.myInventory[2] >= 0) &&
                         (action.repeat * action.deltas[3] + state.myInventory[3] >= 0);
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
    default:
        throw runtime_error("Unhandled action type in canBuy: " + showActionType(action.actionType));
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

        if (a.actionType == ActionType::CAST && a.repeatable)
        {
            for (int i = 1; i < 10; i++)
            {
                Action repeatedAction = a;
                repeatedAction.repeat = i;

                if (canBuy(s, repeatedAction))
                {
                    dest.push_back(repeatedAction);
                } else {
                    break;
                }
            }
        }
        else
        {
            if (canBuy(s, a))
            {
                dest.push_back(a);
            }
        }
    }

    if (canRest)
    {
        RestAction restAction;
        dest.push_back(restAction);
    }
}

bool atLeastOneInvIsUseful(State &state, Action &cast)
{
    for (int i = 0; i < 4; i++)
    {
        if (cast.deltas[i] > 0)
        {
            for (Action &a : state.actions)
            {
                if (a.actionType == ActionType::BREW && a.deltas[i] < 0 && state.myInventory[i] < (-a.deltas[i]))
                {
                    return true;
                }
            }
        }
    }
    return false;
}

void pruneActions(vector<Action> &actions, State &s)
{
    bool canRest = false;
    for (auto &a : actions)
    {
        if (a.actionType == ActionType::REST)
        {
            canRest = true;
            break;
        }
    }

    bool someIsUseful = false;
    for (auto &a : actions)
    {
        if (a.actionType == ActionType::CAST && atLeastOneInvIsUseful(s, a))
        {
            someIsUseful = true;
            break;
        }
    }

    actions.erase(remove_if(actions.begin(), actions.end(), [someIsUseful, canRest, &s](Action &a) {
                      return !(a.actionType != ActionType::CAST || (!someIsUseful && !canRest) || atLeastOneInvIsUseful(s, a));
                  }),
                  actions.end());
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

void throwIfTimeout(int curMaxDepth)
{
    auto elapsed = millis() - start;

    if (elapsed >= TIMEOUT_MS)
    {
        debug("Elapsed ( " + (to_string(elapsed)) + " > " + to_string(TIMEOUT_MS) + " ms) with finished depth " + to_string(curMaxDepth - 1));
        throw runtime_error("No more time");
    }
}

void decideAction(State &state)
{
    vector<pair<Action, double>> sequenceOfActionToState;

    try
    {
        for (int curMaxDepth = 1;; curMaxDepth++)
        {
            vector<pair<Action, double>> curSequenceOfActionToState;
            function<void(vector<Action> &, State &, int)> aux = [&curSequenceOfActionToState, &aux, curMaxDepth](vector<Action> &history, State &s, int depth) mutable {
                countVisitedNodes++;
                throwIfTimeout(curMaxDepth);

                vector<Action> buyable;
                getAllValidActions(s, buyable);
                pruneActions(buyable, s);

                if (!history.empty() && (buyable.empty() || depth == 0))
                {
                    curSequenceOfActionToState.push_back(pair{history.at(0), scoreState(s)});
                    // debug("sequenceOfActionToState: " + to_string(sequenceOfActionToState.size()));
                }
                else
                {
                    vector<tuple<vector<Action>, State, double>> childsStates;

                    // debug("buyable length: " + to_string(buyable.size()));

                    for (auto &a : buyable)
                    {
                        throwIfTimeout(curMaxDepth);
                        State newState = s;
                        playAction(s, a, newState);
                        vector<Action> newHistory = history;
                        newHistory.push_back(a);
                        double score = scoreState(newState);
                        childsStates.push_back({newHistory, newState, score});
                    }

                    sumChildNodes += childsStates.size();
                    countChildNodes++;

                    sort(childsStates.begin(), childsStates.end(), [](tuple<vector<Action>, State, double> a, tuple<vector<Action>, State, double> b) {
                        return get<2>(a) > get<2>(b);
                    });

                    for (int i = 0; i < min(MAX_CHILDS_PER_NODE, (int)childsStates.size()); i++)
                    {
                        aux(get<0>(childsStates[i]), get<1>(childsStates[i]), depth - 1);
                    }
                }
            };

            vector<Action> baseHistory = vector<Action>{};
            aux(baseHistory, state, curMaxDepth);

            sequenceOfActionToState = curSequenceOfActionToState;
        }
    }
    catch (const runtime_error &e)
    {
        // nothing to do
    }

    debug("final sequenceOfActionToState: " + to_string(sequenceOfActionToState.size()));

    debug("average childs by node: " + to_string((double)sumChildNodes / (double)countChildNodes) + ", visited nodes: " + to_string(countVisitedNodes));

    sort(sequenceOfActionToState.begin(), sequenceOfActionToState.end(), [](pair<Action, double> a, pair<Action, double> b) {
        return get<1>(a) > get<1>(b);
    });

    if (!sequenceOfActionToState.empty())
    {
        pair<Action, double> &pickedSequenceToState = sequenceOfActionToState.at(0);
        Action &firstAction = get<0>(pickedSequenceToState);
        if (firstAction.actionType == ActionType::CAST || firstAction.actionType == ActionType::BREW)
        {
            sendBrewCast(firstAction);
        }
        else if (firstAction.actionType == ActionType::REST)
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
        for(int i=0;i<action.repeatable;i++){
            addInventoryDiff(newState.myInventory, action.deltas);
        }
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
    send(showActionType(action.actionType) + " " + to_string(action.actionId) + " " + to_string(action.repeat));
}

void debug(string message)
{
    cerr << message << endl;
}