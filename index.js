/*global readline*/

const INV_WEIGHTS = [1, 2, 3, 4]

class State {
    constructor(actions, myInventory, opponentInventory, myScore, opponentScore) {
        this.actions = actions
        this.myInventory = myInventory
        this.opponentInventory = opponentInventory
        this.myScore = myScore
        this.opponentScore = opponentScore
    }
}

class Action {
    constructor(actionId, actionType, deltas, price, tomeIndex, taxCount, castable, repeatable) {
        this.actionId = actionId
        this.actionType = actionType
        this.deltas = deltas
        this.price = price
        this.tomeIndex = tomeIndex
        this.taxCount = taxCount
        this.castable = castable
        this.repeatable = repeatable
    }
}

// eslint-disable-next-line no-constant-condition
while (true) {
    const actionCount = parseInt(readline()); // the number of spells and recipes in play

    let actions = []
    for (let i = 0; i < actionCount; i++) {
        var inputs = readline().split(' ');
        const actionId = parseInt(inputs[0]); // the unique ID of this spell or recipe
        const actionType = inputs[1]; // in the first league: BREW; later: CAST, OPPONENT_CAST, LEARN, BREW
        const delta0 = parseInt(inputs[2]); // tier-0 ingredient change
        const delta1 = parseInt(inputs[3]); // tier-1 ingredient change
        const delta2 = parseInt(inputs[4]); // tier-2 ingredient change
        const delta3 = parseInt(inputs[5]); // tier-3 ingredient change
        const price = parseInt(inputs[6]); // the price in rupees if this is a potion
        const tomeIndex = parseInt(inputs[7]); // in the first two leagues: always 0; later: the index in the tome if this is a tome spell, equal to the read-ahead tax
        const taxCount = parseInt(inputs[8]); // in the first two leagues: always 0; later: the amount of taxed tier-0 ingredients you gain from learning this spell
        const castable = inputs[9] !== '0'; // in the first league: always 0; later: 1 if this is a castable player spell
        const repeatable = inputs[10] !== '0'; // for the first two leagues: always 0; later: 1 if this is a repeatable player spell
        actions.push(new Action(actionId, actionType, [delta0, delta1, delta2, delta3], price, tomeIndex, taxCount, castable, repeatable))
    }

    let myInventory
    let opponentInventory
    let myScore
    let opponentScore

    for (let i = 0; i < 2; i++) {
        // eslint-disable-next-line no-redeclare
        var inputs = readline().split(' ');
        const inv0 = parseInt(inputs[0]); // tier-0 ingredients in inventory
        const inv1 = parseInt(inputs[1]);
        const inv2 = parseInt(inputs[2]);
        const inv3 = parseInt(inputs[3]);
        const score = parseInt(inputs[4]); // amount of rupees

        let inv = [inv0, inv1, inv2, inv3]

        if (i == 0) {
            myInventory = inv
            myScore = score
        } else {
            opponentInventory = inv
            opponentScore = inv
        }

    }

    let state = new State(actions, myInventory, opponentInventory, myScore, opponentScore)

    // debug(state)

    // Write an action using console.log()
    // To debug: console.error('Debug messages...');

    let action = decideAction(state)

    // in the first league: BREW <id> | WAIT; later: BREW <id> | CAST <id> [<times>] | LEARN <id> | REST | WAIT
    send(`${action.actionType} ${action.actionId}`);
}

function debug(content) {
    console.error(JSON.stringify(content, null, 2))
}

function send(output) {
    console.log(output)
}

function score(action) {
    let cost = sum(INV_WEIGHTS.map((w, i) => w * -action.deltas[i]))
    return action.price / cost
}

function decideAction(state) {
    let sorted = [...state.actions]
        .map(a => [a, score(a)])
        .sort((a, b) => b[1] - a[1])
    debug(sorted)
    return sorted[0][0]
}

function range(n) {
    return [...Array(n).keys]
}

function sum(arr) {
    return arr.reduce((a, c) => a + c, 0)
}