/*global readline*/

const INV_WEIGHTS = [1, 1, 1, 1]

function newState(actions, myInventory, opponentInventory, myScore, opponentScore) {
    return {
        actions: actions,
        myInventory: myInventory,
        opponentInventory: opponentInventory,
        myScore: myScore,
        opponentScore: opponentScore
    }
}

function newAction(actionId, actionType, deltas, price, tomeIndex, taxCount, castable, repeatable) {
    return {
        actionId: actionId,
        actionType: actionType,
        deltas: deltas,
        price: price,
        tomeIndex: tomeIndex,
        taxCount: taxCount,
        castable: castable,
        repeatable: repeatable
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
        actions.push(newAction(actionId, actionType, [delta0, delta1, delta2, delta3], price, tomeIndex, taxCount, castable, repeatable))
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
            opponentScore = score
        }

    }

    let state = newState(actions, myInventory, opponentInventory, myScore, opponentScore)

    // debug(state)

    // Write an action using console.log()
    // To debug: console.error('Debug messages...');

    decideAction(state)

    // in the first league: BREW <id> | WAIT; later: BREW <id> | CAST <id> [<times>] | LEARN <id> | REST | WAIT

}

function debug(msg, content) {
    console.error(`${msg}: ` + JSON.stringify(content))
}

function send(output) {
    console.log(output)
}

function sendWait() {
    send('WAIT')
}

function sendRest() {
    send('REST')
}

function sendBrewCast(action) {
    send(`${action.actionType} ${action.actionId}`)
}

function scoreSide(score, inventory) {
    return score * 1000 + sum(inventory)
}

function scoreState(s) {
    return scoreSide(s.myScore, s.myInventory) - scoreSide(s.opponentScore, s.opponentInventory)
}

/**
 * 
 * @param {State} state 
 * @param {Action} action 
 */
function canBuy(state, action) {
    let countInv = sum(state.myInventory)

    switch (action.actionType) {
        case 'OPPONENT_CAST':
            return false
        case 'CAST':
            {
                let countDeltas = sum(action.deltas)
                let enoughSpace = countInv + countDeltas <= 10
                let enoughInv = (action.deltas[0] + state.myInventory[0] >= 0) &&
                    (action.deltas[1] + state.myInventory[1] >= 0) &&
                    (action.deltas[2] + state.myInventory[2] >= 0) &&
                    (action.deltas[3] + state.myInventory[3] >= 0)
                return action.castable && enoughInv && enoughSpace
            }
        case 'BREW':
            {
                let countDeltas = sum(action.deltas)
                let enoughSpace = countInv + countDeltas <= 10
                let enoughInv = (action.deltas[0] + state.myInventory[0] >= 0) &&
                    (action.deltas[1] + state.myInventory[1] >= 0) &&
                    (action.deltas[2] + state.myInventory[2] >= 0) &&
                    (action.deltas[3] + state.myInventory[3] >= 0)
                return enoughInv && enoughSpace
            }
        case 'LEARN':
            return false
    }

}

/**
 * 
 * @param {Object} s 
 */
function getAllValidActions(s) {
    return [...s.actions].filter(a => canBuy(s, a))
}

function pruneActions(actions, s, canRest){
    let someIsUseful = actions.some(a => a.actionType == 'CAST' && atLeastOneInvIsUseful(s, a))

    return actions.filter(a =>
        (a.actionType != 'CAST' || (!someIsUseful && !canRest) || atLeastOneInvIsUseful(s, a))
    )
}

function addArray(arr, item){
    let res = [...arr]
    res.push(item)
    return res
}

/**
 * 
 * @param {Object} s 
 */
function decideAction(state) {
    let MAX_DEPTH = 2 

    let sequenceOfActionToState = []

    function aux(history, s, depth){
        let buyable = getAllValidActions(s)
        let canRest = s.actions.some(a => a.actionType == 'CAST' && !a.castable)
        let buyableAndUseful = pruneActions(buyable, s, canRest)
        // debug("buyableAndUseful", buyableAndUseful)

        if(history.length > 0 && (buyableAndUseful.length == 0 || depth == 0)){
            sequenceOfActionToState.push([history, s, scoreState(s)])
        } else {
            buyableAndUseful.forEach(a => {
                let newState = playAction(s, a)
                // debug('newState', newState)

                let newHistory = addArray(history, a)
                // debug('newHistory', newState)

                aux(newHistory, newState, depth - 1)
            })
        }
    }

    let canRest = state.actions.some(a => a.actionType == 'CAST' && !a.castable)

    aux([], state, MAX_DEPTH)

    // debug('sequenceOfActionToState', sequenceOfActionToState)

    let pickedSequenceToState = maxBy(sequenceOfActionToState, item => item[2])

    debug("picked", pickedSequenceToState)

    if (pickedSequenceToState && (pickedSequenceToState[0][0].actionType == 'CAST' || pickedSequenceToState[0][0].actionType == 'BREW')) {
        sendBrewCast(pickedSequenceToState[0][0])
    } else if (canRest) {
        sendRest()
    } else {
        sendWait()
    }
}

function isEmpty(arr) {
    return arr.length == 0
}

function maxBy(arr, scoreFn) {
    if (isEmpty(arr)) {
        return null
    }
    let best
    let bestScore
    arr.forEach(e => {
        let s = scoreFn(e)
        if (best == undefined || s > bestScore) {
            best = e
            bestScore = s
        }
    });
    return best
}

function recursiveDeepCopy(obj) {
    return Object.keys(obj).reduce((v, d) => Object.assign(v, {
        [d]: (obj[d].constructor === Object) ? recursiveDeepCopy(obj[d]) : obj[d]
    }), {});
}

function playAction(state, action) {
    let newState = recursiveDeepCopy(state)
    switch (action.actionType) {
        case 'CAST':
            {
                let actionInNewState = newState.actions.find(e => e.actionId == action.actionId)
                actionInNewState.castable = false
                newState.myInventory = addInventoryDiff(newState.myInventory, action.deltas)
            }
            break
        case 'BREW':
            newState.actions = newState.actions.filter(e => e.actionId != action.actionId)
            newState.myInventory = addInventoryDiff(newState.myInventory, action.deltas)
            newState.myScore += action.price
            break
        default:
            throw new Error(`Unhandled action ${action}`)
    }
    return newState
}

function addInventoryDiff(inv, diff) {
    return inv.map((e, i) => e + diff[i])
}

function randomInArray(array) {
    return array[Math.floor(Math.random() * array.length)]
}

function range(n) {
    return [...Array(n).keys]
}

function sum(arr) {
    return arr.reduce((a, c) => a + c, 0)
}

/**
 * 
 * @param {State} state 
 * @param {Action} cast 
 */
function atLeastOneInvIsUseful(state, cast) {
    return INV_WEIGHTS.some((w, i) =>
        cast.deltas[i] > 0 && state.actions.some(a =>
            a.actionType == 'BREW' && a.deltas[i] < 0 && state.myInventory[i] < (-a.deltas[i])
        )
    )
}