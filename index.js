/*global readline*/

const INV_WEIGHTS = [1, 1, 1, 1]

// eslint-disable-next-line no-constant-condition
let turn = 0
while (true) {
    if (turn < 10) {
        let freeLearn = state.actions.filter(a => a.actionType == actionTypes.LEARN)[0]
        send(`LEARN ${freeLearn.actionId}`)
    } else {
        decideAction(state)
    }

    // in the first league: BREW <id> | WAIT; later: BREW <id> | CAST <id> [<times>] | LEARN <id> | REST | WAIT
    turn++
}

function debug(msg, content) {
    console.error(`${msg}:\n` + JSON.stringify(content))
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
 * @param {Object} state 
 * @param {Object} action 
 */
function canBuy(state, action) {
    let countInv = sum(state.myInventory)

    switch (action.actionType) {
        case actionTypes.OPPONENT_CAST:
            return false
        case actionTypes.CAST:
            {
                let countDeltas = sum(action.deltas)
                let enoughSpace = countInv + countDeltas <= 10
                let enoughInv = (action.deltas[0] + state.myInventory[0] >= 0) &&
                    (action.deltas[1] + state.myInventory[1] >= 0) &&
                    (action.deltas[2] + state.myInventory[2] >= 0) &&
                    (action.deltas[3] + state.myInventory[3] >= 0)
                return action.castable && enoughInv && enoughSpace
            }
        case actionTypes.BREW:
            {
                let countDeltas = sum(action.deltas)
                let enoughSpace = countInv + countDeltas <= 10
                let enoughInv = (action.deltas[0] + state.myInventory[0] >= 0) &&
                    (action.deltas[1] + state.myInventory[1] >= 0) &&
                    (action.deltas[2] + state.myInventory[2] >= 0) &&
                    (action.deltas[3] + state.myInventory[3] >= 0)
                return enoughInv && enoughSpace
            }
        case actionTypes.LEARN:
            return false
        default:
            throw new Error(`Unhandled action type: ${action.actionType}`)
    }

}

/**
 * 
 * @param {Object} s 
 */
function getAllValidActions(s) {
    let canRest = s.actions.some(a => a.actionType == 'CAST' && !a.castable)
    return [...s.actions].filter(a => canBuy(s, a)).concat(canRest ? [restAction] : [])
}

function pruneActions(actions, s) {
    let canRest = actions.some(a => a.actionId == restAction.actionId)
    let someIsUseful = actions.some(a => a.actionType == 'CAST' && atLeastOneInvIsUseful(s, a))

    return actions.filter(a =>
        (a.actionType != 'CAST' || (!someIsUseful && !canRest) || atLeastOneInvIsUseful(s, a))
    )
}

function addArray(arr, item) {
    let res = [...arr]
    res.push(item)
    return res
}

/**
 * 
 * @param {Object} s 
 */
function decideAction(state) {
    let MAX_DEPTH = 5

    let sequenceOfActionToState = []

    function aux(history, s, depth) {
        let buyable = getAllValidActions(s)
        let buyableAndUseful = pruneActions(buyable, s)

        if (history.length > 0 && (buyableAndUseful.length == 0 || depth == 0)) {
            sequenceOfActionToState.push([history, s, scoreState(s)])
        } else {
            sortBy(buyableAndUseful.map(a => {
                let newState = playAction(s, a)
                let newHistory = addArray(history, a)
                let score = scoreState(newState)
                return [newHistory, newState, score]
            }), e => -e[2])
                .slice(0, 3)
                .map(e => {
                    let [newHistory, newState, score] = e
                    aux(newHistory, newState, depth - 1)
                })
        }
    }

    aux([], state, MAX_DEPTH)

    debug('sequenceOfActionToState count:', sequenceOfActionToState.length)

    let pickedSequenceToState = maxBy(sequenceOfActionToState, item => item[2])

    debug("picked", pickedSequenceToState)

    if (pickedSequenceToState && (pickedSequenceToState[0][0].actionType == 'CAST' || pickedSequenceToState[0][0].actionType == 'BREW')) {
        sendBrewCast(pickedSequenceToState[0][0])
    } else if (pickedSequenceToState && (pickedSequenceToState[0][0].actionType == restAction.actionType)) {
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

function sortBy(arr, scoreFn) {
    arr.sort((a, b) => scoreFn(a) - scoreFn(b))
    return arr
}

function recursiveDeepCopy(obj) {
    return Object.keys(obj).reduce((v, d) => Object.assign(v, {
        [d]: (obj[d].constructor === Object) ? recursiveDeepCopy(obj[d]) : obj[d]
    }), {});
}

function playAction(state, action) {
    let newState = recursiveDeepCopy(state)
    switch (action.actionType) {
        case actionTypes.CAST:
            {
                let actionInNewState = newState.actions.find(e => e.actionId == action.actionId)
                actionInNewState.castable = false
                newState.myInventory = addInventoryDiff(newState.myInventory, action.deltas)
            }
            break
        case actionTypes.BREW:
            newState.actions = newState.actions.filter(e => e.actionId != action.actionId)
            newState.myInventory = addInventoryDiff(newState.myInventory, action.deltas)
            newState.myScore += action.price + action.tomeIndex
            newState.actions
                .filter(a => a.actionType == actionTypes.BREW)
                .forEach((a, i) => {
                    if (i == 0) {
                        a.tomeIndex = 3
                    } else if (i == 1) {
                        a.tomeIndex = 1
                    } else {
                        a.tomeIndex = 0
                    }
                })
            break
        case actionTypes.REST:
            newState.actions.forEach(a => {
                if (a.actionType == actionTypes.CAST && !a.castable) {
                    a.castable = true
                }
            })
            break
        default:
            throw new Error(`Unhandled action ${JSON.stringify(action)}`)
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