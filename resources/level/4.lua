function checkHero(name, unit, replacement)
    if not hasHero(name) then
        createUnit(unit.x, unit.y, replacement, 0)
        unit.destroy()
    end
end
function heroTalk(unit, name, team, message)
    print(name)
    if unit.valid then
        moveCursor(unit.x, unit.y)
        showMessage(message, name, team)
        while messageOpen() do yield() end
        return true
    end
    return false
end
function spawnUnitNear(x, y, key, team)
    print(x, y, key, getUnitAt(x, y))
    if getUnitAt(x, y) ~= nil then
        if getUnitAt(x - 1, y) == nil then x = x - 1
        elseif getUnitAt(x + 1, y) == nil then x = x + 1
        elseif getUnitAt(x, y - 1) == nil then y = y - 1
        elseif getUnitAt(x, y + 1) == nil then y = y + 1
        elseif getUnitAt(x - 1, y + 1) == nil then x = x - 1; y = y + 1
        elseif getUnitAt(x + 1, y + 1) == nil then x = x + 1; y = y + 1
        elseif getUnitAt(x - 1, y - 1) == nil then x = x - 1; y = y - 1
        elseif getUnitAt(x + 1, y - 1) == nil then x = x + 1; y = y - 1
        end
    end
    print(x, y, key)
    return createUnit(x, y, key, team)
end

checkHero("Apollo", ApolloPlayer, "Hero")
checkHero("Artemis", ArtemisPlayer, "Pirate")
checkHero("Poseidon", PoseidonPlayer, "Pirate")
checkHero("Heracles", HeraclesPlayer, "Hero")
yield() -- update valid flags

heroTalk(ZeusAI, "Zeus", 1, [[Zeus:
So, you've finally arrived. I was starting to worry.]])
heroTalk(AphroditePlayer, "Aphrodite", 0, [[Aphrodite:
The road certainly wasn't easy. So, your summons, Lord Zeus?]])
heroTalk(ZeusAI, "Zeus", 1, [[Zeus:
I was thinking about "expanding the family".]])
function allDot()
    heroTalk(AphroditePlayer, "Aphrodite", 0, [[Aphrodite:
...]])
    heroTalk(ApolloPlayer, "Apollo", 0, [[Apollo:
...]])
    heroTalk(ArtemisPlayer, "Artemis", 0, [[Artemis:
...]])
    heroTalk(PoseidonPlayer, "Poseidon", 0, [[Poseidon:
...]])
    heroTalk(HeraclesPlayer, "Heracles", 0, [[Heracles:
...]])
end
allDot()
if not heroTalk(ArtemisPlayer, "Artemis", 0, [[Artemis:
You called us all the way up here...]]) then
    heroTalk(AphroditePlayer, "Aphrodite", 0, [[Aphrodite:
You called us all the way up here...]])
end
if not heroTalk(ApolloPlayer, "Apollo", 0, [[Apollo:
Made us do battle with one another...]]) then
    heroTalk(AphroditePlayer, "Aphrodite", 0, [[Aphrodite:
Made us do battle with one another...]])
end
if not heroTalk(HeraclesPlayer, "Heracles", 0, [[Heracles:
Threw away the lives of countless heroes...]]) then
    heroTalk(AphroditePlayer, "Aphrodite", 0, [[Aphrodite:
Threw away the lives of countless heroes...]])
end
heroTalk(AphroditePlayer, "Aphrodite", 0, [[Aphrodite:
For THAT!?]])
heroTalk(ZeusAI, "Zeus", 1, [[Zeus:
Yes. Though from your reaction,]])
heroTalk(ZeusAI, "Zeus", 1, [[Zeus:
I take it I'll have to turn on the charm to convince you?]])
allDot()
heroTalk(AphroditePlayer, "Aphrodite", 0, [[Aphrodite:
... Only you, Lord Zeus...]])
heroTalk(AphroditePlayer, "Aphrodite", 0, [[Aphrodite:
This mockery will not go unpunished!]])
showMessage([[Tutorial:
Claim the Throne of Olympus with any God!]], "", 0)
while messageOpen() do yield() end

function onFinish(unit)
    if not unit.valid then return false end
    if not unit.isPlayer() then return false end
    return unit.x == -1 and unit.y == 12
end

function onTurnEnd()
    if onFinish(AphroditePlayer) then
        heroTalk(AphroditePlayer, "Aphrodite", 0, [[Aphrodite:
That wasn't so hard.]])
        showMessage([[Zeus:
Bested by the Goddess of Love...]], "Zeus", 1)
        while messageOpen() do yield() end
        heroTalk(AphroditePlayer, "Aphrodite", 0, [[Aphrodite:
Of course! There is no greater force in the world.]])
        heroTalk(AphroditePlayer, "Aphrodite", 0, [[Aphrodite:
Now then, let us begin a glorious new era of love!]])
        victory()
    end
    if onFinish(ArtemisPlayer) then
        showMessage([[Zeus:
I can't believe it, Artemis?]], "Zeus", 1)
        while messageOpen() do yield() end
        heroTalk(ArtemisPlayer, "Artemis", 0, [[Artemis:
Don't be so shocked.]])
        heroTalk(ArtemisPlayer, "Artemis", 0, [[Artemis:
It's about time a woman took charge around here.]])
        victory()
    end
    if onFinish(ApolloPlayer) then
        heroTalk(ApolloPlayer, "Apollo", 0, [[Apollo:
There was never any doubt I would outshine the competition.]])
        showMessage([[Zeus:
But... You are the sun!]], "Zeus", 1)
        while messageOpen() do yield() end
        heroTalk(ApolloPlayer, "Apollo", 0, [[Apollo:
Yes! And the sun should always rule the skies.]])
        heroTalk(ApolloPlayer, "Apollo", 0, [[Apollo:
Let's ride into a bright new future!]])
        victory()
    end
    if onFinish(PoseidonPlayer) then
        showMessage([[Zeus:
But, you were given the seas, Poseidon!]], "Zeus", 1)
        while messageOpen() do yield() end
        heroTalk(PoseidonPlayer, "Poseidon", 0, [[Poseidon:
Yes, brother, but you have ruled the land and sky poorly.]])
        heroTalk(PoseidonPlayer, "Poseidon", 0, [[Poseidon:
Perhaps it's time for one God to rule the world?]])
        victory()
    end
    if onFinish(HeraclesPlayer) then
        showMessage([[Zeus:
Heracles? But you're no god!]], "Zeus", 1)
        while messageOpen() do yield() end
        heroTalk(HeraclesPlayer, "Heracles", 0, [[Heracles:
Your blood runs in my veins, father.]])
        heroTalk(HeraclesPlayer, "Heracles", 0, [[Heracles:
By right, let Olympus uplift all of mankind!]])
        victory()
    end
    if onFinish(ZeusAI) then
        heroTalk(ZeusAI, "Zeus", 0, [[Zeus:
Alright... you may have been right.]])
        heroTalk(ZeusAI, "Zeus", 0, [[Zeus:
This was a little... silly.]])
        heroTalk(AphroditePlayer, "Aphrodite", 0, [[Aphrodite:
Yes. But... no harm done.]])
        heroTalk(AphroditePlayer, "Aphrodite", 0, [[Aphrodite:
We are all family after all.]])
        victory()
    end
end
function onTurnStart()
    if not AphroditePlayer.valid then
        showMessage([[Aphrodite has been defeated. You lost.]], "", 0)
        while messageOpen() do yield() end
        defeat()
        return
    end
end
