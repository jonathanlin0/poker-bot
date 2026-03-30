const SUIT_SYMBOLS = { H: '●', D: '●', C: '●', S: '●' };
const SUIT_CLASSES = { H: 'suit-H', D: 'suit-D', C: 'suit-C', S: 'suit-S' };
const STREET_NAMES = ['Preflop', 'Flop', 'Turn', 'River'];

let username = '';
let currentState = null;

function formatCard(cardStr) {
    if (!cardStr || cardStr.length < 2) return '';
    const suit = cardStr[0];
    const rank = cardStr[1];
    const cls = SUIT_CLASSES[suit] || '';
    const sym = SUIT_SYMBOLS[suit] || '?';
    return `${rank}<span class="${cls}">${sym}</span>`;
}

function formatCards(cards) {
    if (!cards || cards.length === 0) return '';
    return cards.map(formatCard).join(' ');
}

function formatStreetActions(street, streetIdx, playerSeat) {
    let hasUnmatchedBet = (streetIdx === 0);
    return street.map((action, actionIdx) => {
        const t = action.type;
        const amt = action.amount;
        let text;
        if (t === 'b') {
            text = (actionIdx === 0) ? 'SB' : 'BB';
        } else if (t === 'f') {
            text = 'f';
        } else if (t === 'c') {
            text = hasUnmatchedBet ? 'c' : 'ch';
            hasUnmatchedBet = false;
        } else if (t === 'r') {
            hasUnmatchedBet = true;
            text = 'r' + (amt / 2.0).toFixed(1) + 'BB';
        } else if (t === 'a') {
            hasUnmatchedBet = true;
            text = 'a';
        } else {
            text = t;
        }
        const actorSeat = actionIdx % 2;
        const isUser = (actorSeat === playerSeat);
        const cls = isUser ? 'action-user' : 'action-bot';
        return `<span class="${cls}">${text}</span>`;
    }).join(', ');
}

function renderHistory(history, containerId, playerSeat) {
    const container = document.getElementById(containerId);
    container.innerHTML = '';
    if (!history || history.length === 0) {
        container.textContent = 'No actions yet.';
        return;
    }
    history.forEach((street, streetIdx) => {
        if (street.length === 0) return;
        const div = document.createElement('div');
        div.className = 'street-history';
        const label = STREET_NAMES[streetIdx] || 'Street ' + streetIdx;
        div.innerHTML = `<span class="street-label">${label}:</span> ${formatStreetActions(street, streetIdx, playerSeat)}`;
        container.appendChild(div);
    });
}

function renderActions(allActions, validActions) {
    const rowBasic = document.getElementById('row-basic');
    const rowRaises = document.getElementById('row-raises');
    const rowAllin = document.getElementById('row-allin');
    rowBasic.innerHTML = '';
    rowRaises.innerHTML = '';
    rowAllin.innerHTML = '';

    const validLabels = new Map();
    if (validActions) {
        validActions.forEach(va => {
            validLabels.set(va.label, va);
        });
    }

    if (!allActions || allActions.length === 0) return;

    allActions.forEach(label => {
        const btn = document.createElement('button');
        btn.className = 'btn-action';
        btn.textContent = label;

        const validEntry = validLabels.get(label);
        if (!validEntry) {
            btn.disabled = true;
        } else {
            btn.addEventListener('click', () => {
                const actionPayload = { type: validEntry.type };
                if (validEntry.type === 'r' && validEntry.amount !== undefined) {
                    actionPayload.amount = validEntry.amount;
                }
                sendAction(actionPayload);
            });
        }

        if (label === 'f' || label === 'ch' || label === 'c') {
            rowBasic.appendChild(btn);
        } else if (label === 'a') {
            rowAllin.appendChild(btn);
        } else {
            rowRaises.appendChild(btn);
        }
    });
}

function renderProfit(state) {
    if (state.user_profit_bb !== undefined) {
        const up = Number(state.user_profit_bb);
        const bp = Number(state.bot_profit_bb);
        document.getElementById('user-profit').textContent = (up >= 0 ? '+' : '') + up.toFixed(1);
        document.getElementById('bot-profit').textContent = (bp >= 0 ? '+' : '') + bp.toFixed(1);
    }
    if (state.hands_played !== undefined) {
        document.getElementById('hands-played').textContent = state.hands_played;
    }
}

function renderState(state) {
    currentState = state;

    renderProfit(state);

    document.getElementById('position-display').textContent =
        state.player_seat === 0 ? 'SB' : 'BB';

    document.getElementById('player-cards').innerHTML = formatCards(state.player_cards);
    document.getElementById('board-cards').innerHTML = formatCards(state.board);

    document.getElementById('player-stack').textContent =
        Number(state.player_stack).toFixed(1);
    document.getElementById('bot-stack').textContent =
        Number(state.bot_stack).toFixed(1);
    document.getElementById('pot-size').textContent =
        Number(state.pot).toFixed(1);

    renderHistory(state.history, 'current-history', state.player_seat);

    const botRow = document.getElementById('bot-cards-row');
    const resultEl = document.getElementById('result-display');
    const actionButtons = document.getElementById('action-buttons');
    const prompt = document.getElementById('game-prompt');

    if (state.game_over) {
        botRow.hidden = false;
        document.getElementById('bot-cards').innerHTML = formatCards(state.bot_cards);

        const result = state.result_bb;
        resultEl.hidden = false;
        if (result > 0) {
            resultEl.textContent = `You won ${result.toFixed(1)} BB`;
            resultEl.className = 'result-display result-win';
        } else if (result < 0) {
            resultEl.textContent = `You lost ${Math.abs(result).toFixed(1)} BB`;
            resultEl.className = 'result-display result-loss';
        } else {
            resultEl.textContent = 'Tie';
            resultEl.className = 'result-display result-tie';
        }

        actionButtons.hidden = true;
        newHandContainer.hidden = true;
        prompt.textContent = 'Hand complete — next hand starting...';

        setTimeout(() => fetchGameState(), 2000);
    } else {
        botRow.hidden = true;
        resultEl.hidden = true;
        actionButtons.hidden = false;
        prompt.textContent = 'Press an action below to play';

        renderActions(state.all_actions, state.valid_actions);
    }

    renderPastHands(state.past_hands);
}

function renderPastHands(pastHands) {
    const container = document.getElementById('past-hands');
    container.innerHTML = '';

    if (!pastHands || pastHands.length === 0) {
        container.textContent = 'No past hands yet.';
        return;
    }

    const wrapper = document.createElement('div');
    wrapper.className = 'past-hands-container';

    pastHands.forEach(hand => {
        const div = document.createElement('div');
        div.className = 'past-hand';

        const pos = hand.player_seat === 0 ? 'SB' : 'BB';
        const resultBB = hand.result_bb;
        let resultClass, resultText;
        if (resultBB > 0) {
            resultClass = 'past-hand-result-positive';
            resultText = '+' + resultBB.toFixed(1) + ' BB';
        } else if (resultBB < 0) {
            resultClass = 'past-hand-result-negative';
            resultText = resultBB.toFixed(1) + ' BB';
        } else {
            resultClass = 'past-hand-result-zero';
            resultText = '0.0 BB';
        }

        let html = `<div class="past-hand-header">${pos} | <span class="${resultClass}">${resultText}</span></div>`;
        html += `<div>You: ${formatCards(hand.player_cards)} | Bot: ${formatCards(hand.bot_cards)}</div>`;
        if (hand.board && hand.board.length > 0) {
            html += `<div>Board: ${formatCards(hand.board)}</div>`;
        }

        const histContainer = document.createElement('div');
        hand.history.forEach((street, sIdx) => {
            if (street.length === 0) return;
            const label = STREET_NAMES[sIdx] || 'Street ' + sIdx;
            histContainer.innerHTML += `<div class="street-history"><span class="street-label">${label}:</span> ${formatStreetActions(street, sIdx, hand.player_seat)}</div>`;
        });

        div.innerHTML = html;
        div.appendChild(histContainer);
        wrapper.appendChild(div);
    });

    container.appendChild(wrapper);
}

function showError(msg) {
    const prompt = document.getElementById('game-prompt');
    prompt.textContent = msg;
    console.error(msg);
}

async function fetchGameState() {
    try {
        const res = await fetch('/hand-state', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ username })
        });
        if (!res.ok) {
            const data = await res.json();
            showError(data.error || 'Failed to load game state');
            return;
        }
        const state = await res.json();
        renderState(state);
    } catch (err) {
        console.error('fetchGameState failed:', err);
        setTimeout(() => fetchGameState(), 1000);
    }
}

async function sendAction(action) {
    const buttons = document.querySelectorAll('.btn-action');
    buttons.forEach(b => b.disabled = true);

    try {
        const res = await fetch('/action', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ username, action })
        });
        const data = await res.json();
        if (!res.ok) {
            showError(data.error || 'Invalid action');
            fetchGameState();
            return;
        }
        if (data.past_hands === undefined && currentState && currentState.past_hands) {
            data.past_hands = currentState.past_hands;
        }
        renderState(data);
    } catch (err) {
        console.error('sendAction failed:', err);
        fetchGameState();
    }
}

document.addEventListener('DOMContentLoaded', () => {
    const pathParts = window.location.pathname.split('/');
    username = decodeURIComponent(pathParts[pathParts.length - 1]);
    if (!username) {
        window.location.href = '/play';
        return;
    }
    const lower = username.toLowerCase();
    if (username !== lower) {
        window.location.replace('/play/' + encodeURIComponent(lower));
        return;
    }
    document.getElementById('username-display').textContent = username;

    fetchGameState();
});
