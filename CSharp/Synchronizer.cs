using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;
using Mirror;

namespace RTS.Unit
{
    [Flags]
    public enum CommandType
    {
        Stop = 1,
        Move = 2,
        Attack = 4,
        Produce = 8,
        EndTurn = 16
    }

    [Flags]
    public enum CommandModifiers
    {
        None = 0,
        AlternateOne = 1,
        AlternateTwo = 2,
        SetRally = 4, // Will read other command present!
        Queue = 8,
    }
    [Serializable]
    public struct Command
    {
        /// <summary>
        /// The command itself.
        /// </summary>
        public CommandType commandType;
        /// <summary>
        /// The modifiers for this command (alternate 1 & 2, rally command, queue).
        /// </summary>
        public CommandModifiers commandModifiers;
        
        /// <summary>
        /// The ID of the unit targeted by this command.
        /// </summary>
        public readonly uint targetUnit;
        /// <summary>
        /// The position in the world targeted by this command.
        /// </summary>
        public readonly Vector2 targetPosition;
        
        public Command(CommandType commandType, CommandModifiers commandModifiers, uint targetUnit, Vector2 targetPosition)
        {
            this.targetUnit = targetUnit;
            this.targetPosition = targetPosition;
            
            this.commandType = commandType;
            this.commandModifiers = commandModifiers;
        }
    }
}

namespace RTS.Game
{
    /* Order of precedence:
     * 1. Engines
     * 2. Guns
     * 3. Factories
     */
    /* List of commands that are valid:
     #. Command -> Alternate Command (Ctrl + Command), Alternate Command 2 (Alt + Command)
     1. Stop
     2. Move, Face Toward
     3. Attack, Aim, Aim At
     4. Set Rally Point
     5. Produce Unit 1, 2, 3
    */ 
    
    [Serializable]
    public struct Input
    {
        /// <summary>
        /// Player ID of which player issued the given command.
        /// </summary>
        public uint issuingPlayer;
        
        /// <summary>
        /// The command to execute on all selected units.
        /// </summary>
        public readonly Unit.Command Command;
        
        /// <summary>
        /// All selected units.
        /// </summary>
        public uint[] selectedUnits;

        public Input(byte commandIndex, 
                     Unit.Command command,
                     Unit.Controller[] selectedUnits)
        {
            // Default the player
            issuingPlayer = 0; // Assigned by server
            // Store the command
            this.Command = command;
            
            // Store how many units are selected
            int selectedUnitsCount = selectedUnits.Length;
            // Instantiate the array of units selected
            this.selectedUnits = new uint[selectedUnitsCount];
            // Populate the list of selected units
            for (int selectionIndex = 0;
                 selectionIndex < selectedUnitsCount;
                 selectionIndex++)
            {
                this.selectedUnits[selectionIndex] = selectedUnits[selectionIndex].id;
            }
        }
    }
    
    public class Synchronizer : NetworkBehaviour
    {
        // SINGLETON REF
        public static Synchronizer Instance;
        
        // CONSTANTS
        /// <summary>
        /// The frequency (times per second) to poll for input. Must be less than physics step frequency. 
        /// </summary>
        private const int StepFrequency = 20;
        /// <summary>
        /// Length of each input interval.
        /// </summary>
        private const float StepLength = 1 / (float) StepFrequency;
        /// <summary>
        /// The amount of time (seconds) to delay each step.
        /// </summary>
        private const float InputBufferLength = 0.10f;
        /// <summary>
        /// The number of input polling steps to delay input by, rounding up
        /// </summary>
        private static readonly int InputDelay = (int) Math.Ceiling(InputBufferLength / StepLength);
        /// <summary>
        /// The maximum number of players allowed into the game at any given time.
        /// </summary>
        public const int PlayerCap = 2;
        
        
        // Todo: Move state to a struct for easy reset?
        
        // FIELDS - STATE, SIMULATION
        /// <summary>
        /// Player numbers by their connectionIDs.
        /// Todo: Switch ConnectionIDs to something more robust.
        /// </summary>
        public static readonly SortedDictionary<uint, Player> Players = new();
        /// <summary>
        /// The number of players active.
        /// </summary>
        public static byte playerCount;
        
        /// <summary>
        /// The current physics frame.
        /// </summary>
        private static int _stepCurrent;
        /// <summary>
        /// Whether the simulation has begun or not.
        /// </summary>
        public static bool started;
        /// <summary>
        /// The time elapsed since the start of the simulation.
        /// </summary>
        public static float currentTime;
        
        /// <summary>
        /// Whether time has advanced this frame or not. Access externally ONLY after time has stepped.
        /// </summary>
        // Todo: If you're thinking of using this outside coroutines, DONT! Leverage OnPhysicsAdvance to save brain problems
        public static bool didTimeAdvance;
        /// <summary>
        /// The amount of time elapsed between the last physics advance, in seconds.
        /// </summary>
        public static float deltaPhysicsTime;
        
        
        // FIELDS - STATE, INPUT
        /// <summary>
        /// All currently pending/scheduled inputs.
        /// </summary>
        private static readonly SortedList<int, List<Input>> ScheduledInputs = new();
        /// <summary>
        /// All enqueued inputs waiting to be sent to the server on the next input sample.
        /// </summary>
        private static readonly Queue<Input> PendingInputs = new();
        /// <summary>
        /// The input states the server has received.
        /// </summary>
        private static readonly SortedList<int, byte> InputsReceived = new();
        
        // Todo: Make non-const if we want this to be optional. Or just remove it.
        /// <summary>
        /// Whether to store inputs for a replay.
        /// </summary>
        private const bool RecordInputs = false;
        /// <summary>
        /// A history of all inputs executed thus far.
        /// Used to record a game for replay.
        /// </summary>
        private static readonly SortedList<int, Input[]> recordedInputs = new();

        
        // METHOD VARIABLES, MICRO-OPTIMIZATION
        private static uint _issuingPlayerID;
        private static float _timeOfNextStep; // Is this even used???
        
        
        // PROPERTIES
        private static bool ReadyToExecute => InputsReceived.TryGetValue(_stepCurrent, out var inputCount) 
                                              && inputCount == PlayerCap;
        
        
        // PRIVATE METHODS - UNITY
        private void Awake()
        {
            // Set the singleton ref.
            Instance = this;
            // Take manual control of physics
            Physics2D.simulationMode = SimulationMode2D.Script;
        }

        
        // METHODS - STATE
        /// <summary>
        /// Activates/initalizes network stepping on the executing user.
        /// </summary>
        public void BeginSynchronization()
        {
            // Flag the simulation to go
            started = true;
            
            if (isClient)
            {
                // Send the first inputs to be scheduled
                for (int startStep = 0; startStep < InputDelay; startStep++)
                {
                    // Schedule the padding inputs to kick off the simulation
                    _ScheduleInputs(Array.Empty<Input>(), startStep - InputDelay);
                }
            }
        }
        /// <summary>
        /// Refreshes all variables pertaining to the simulation sync, to a state as-if new.
        /// </summary>
        public static void ResetSynchronization()
        {
            // Block any further simulation
            started = false;
            // Reset all recorded inputs
            recordedInputs.Clear();
            // Reset all received inputs
            InputsReceived.Clear();
            // Reset all pending inputs
            PendingInputs.Clear();
            // Reset all scheduled inputs
            ScheduledInputs.Clear();
            // Default the deltaTime
            deltaPhysicsTime = 0;
            // Default the current time
            currentTime = 0;
            // Default the step
            _stepCurrent = 0;
            // Default whether time advanced
            didTimeAdvance = false;
            // Todo: Add checks to abort coroutines if started == false, or use reflection and clean them up?
        }

        /// <summary>
        /// Begins a count to begin the game in 0.1 seconds // Todo: MFN.
        /// </summary>
        /// <returns>Enumerator after 0.1 seconds.</returns>
        public static IEnumerator BeginStartCountdown()
        {
            // Todo: Set an MFN, DEBUG at 0.1f
            // Begin a 0.1-second countdown
            yield return new WaitForSeconds(0.1f);
            // Reset the game
            Game.Manager.ResetGame();
            // Start the game
            Game.Manager.StartGame();
        }

        // METHODS - PLAYERS
        /// <summary>
        /// Register the provided player to the server.
        /// </summary>
        /// <param name="networkIdentity">The NetworkIdentity of the player to register.</param>
        /// <param name="player">The player who's being registered.</param>
        public static void RegisterPlayer(NetworkIdentity networkIdentity, Player player)
        {
            // Store this player to the Players list
            Players.Add(player.netId + 100, player);
            
            // Assign the team of this player
            player.team = networkIdentity.netId + 100;
            
            // Assign the player's filters' depths and masks
            player.contactFilterHostiles.minDepth = player.contactFilterHostiles.maxDepth = player.team;
            player.contactFilterHostiles.layerMask = LayerMask.NameToLayer("Ships");
            player.contactFilterFriendlies.minDepth = player.contactFilterHostiles.maxDepth = player.team;
            player.contactFilterFriendlies.layerMask = LayerMask.NameToLayer("Ships");
            
            // Increment the player count
            playerCount++;

            // When all players are in the lobby, activate synchronization.
            if (playerCount == PlayerCap)
            {
                // Start a countdown to begin the game
                Instance.StartCoroutine(BeginStartCountdown());
            }
        }
        /// <summary>
        /// Reset all players to their default values.
        /// </summary>
        public static void ResetPlayers()
        {
            // Reset each player
            foreach (Player player in Players.Values)
            {
                // Reset this player
                player.ResetPlayer();
            }
        }
        
        // METHODS - LOCKSTEP
        /// <summary>
        /// Advances time, advancing the simulation and executing inputs for the provided time interval. 
        /// </summary>
        /// <param name="deltaTimeBudget">The amount of time to pass.</param>
        /// <param name="collectInputs">Whether to collect inputs after a lockstep.</param>
        public static void SimulateTime(float deltaTimeBudget, bool collectInputs = true)
        {
            // Compute how much time until the next step occurs
            float timeTillStep = _timeOfNextStep - currentTime;

            // Default time having advanced to true.
            didTimeAdvance = true;
            // Update the amount of elapsed time
            deltaPhysicsTime = deltaTimeBudget;

            // Check if it's time to execute the next input step, or catch up if we're past that point
            if (deltaTimeBudget > timeTillStep)
            {
                // Check if players aren't ready to execute yet!
                if (!ReadyToExecute)
                {
                    // Break out of the current step
                    Debug.LogWarning($"Inputs for step {_stepCurrent} not received!");
                    // Flag time as having not moved forward.
                    didTimeAdvance = false;
                    // Update our deltaPhysicsTime
                    deltaPhysicsTime = 0;
                    return;
                }

                // Advance time up till the step
                _AdvanceTime(timeTillStep);

                // Perform the lockstep
                Instance._PerformLockstep(collectInputs);

                // Adjust the deltaTime budget
                deltaTimeBudget -= timeTillStep;

                // Advance the remaining time
                _AdvanceTime(deltaTimeBudget);
            }
            else
            {
                // Advance time normally
                _AdvanceTime(deltaTimeBudget);
            }
        }
        private static void _AdvanceTime(float deltaTime)
        {
            // Execute OnAdvancePhysics
            Manager.OnAdvancePhysics(deltaTime);
            // Advance physics
            Physics2D.Simulate(deltaTime);
            // Store the passed time
            currentTime += deltaTime;
        }
        /// <summary>
        /// Executes inputs and reschedules new ones, if requested.
        /// </summary>
        /// <param name="collectInputs">Whether to re-collect inputs. False if replaying/not a player.</param>
        private void _PerformLockstep(bool collectInputs)
        {
            // Execute all inputs
            _ExecuteInputs(stepCurrent: _stepCurrent);
            
            // Check if all players are readied up
            _TryAdvanceTurn();
            
            // Send out state update to the server
            if (isClient && collectInputs)
            {
                // Send inputs to be scheduled
                _ScheduleInputs(PendingInputs.ToArray(), _stepCurrent);
                
                // Reset them after sending
                PendingInputs.Clear();
            }
            
            // Todo: Investigate making a wrapping method to prevent overflows after long periods of time- also float precision drifting
            // Step forward
            _stepCurrent++;
            // Update the time of the next step
            _timeOfNextStep = (_stepCurrent + 1) * StepLength;
        }
        
        
        // PRIVATE METHODS - INPUT
        /// <summary>
        /// Enqueues the given command locally, pending sending in the next input batch for scheduling on the server.
        /// </summary>
        /// <param name="command">The command to be enqueued.</param>
        /// <param name="selectedUnits">The units to be commanded.</param>
        public static void EnqueueCommand(Unit.Command command, List<Unit.Controller> selectedUnits)
        {
            // Enqueue the command, storing the input data
            PendingInputs.Enqueue(item: new Input(commandIndex: (byte)PendingInputs.Count,
                                                  command: command,
                                                  selectedUnits: selectedUnits.ToArray()));
        }
        /// <summary>
        /// Execute all inputs scheduled at the provided step.
        /// </summary>
        /// <param name="stepCurrent">The step at which to execute inputs.</param>
        private static void _ExecuteInputs(int stepCurrent)
        {
            // Todo: Consider refactoring to send individual inputs, and instead de-sync the individual player who's dropping inputs
            // (rather than waiting for all of them to arrive)
            
            // Grab all inputs
            List<Input> pendingInputs = ScheduledInputs[stepCurrent];

            // Execute each input that's scheduled
            foreach (var input in pendingInputs)
            {
                // Pass the input to be executed on the game manager
                Manager.ExecuteInput(input);
            }
            
            // Remove the current step from the scheduled inputs
            ScheduledInputs.Remove(stepCurrent);
            // Remove the number of tracked inputs for target step
            InputsReceived.Remove(stepCurrent);
        }
        /// <summary>
        /// Submits the provided inputs for the unlocking.
        /// </summary>
        /// <param name="inputs">The inputs to schedule.</param>
        /// <param name="sourceStep">The input step the given input is coming from.</param>
        /// <param name="sender">Networking parameter, disregard.</param>
        [Command (requiresAuthority = false)]
        private void _ScheduleInputs(Input[] inputs, int sourceStep, NetworkConnectionToClient sender = null)
        {
            // Compute the delay
            int targetInputStep = sourceStep + InputDelay;
            // Store the executing player's ID
            uint playerID = sender.identity.netId + 100; // Note: The day this is null, we have far worse problems.
            
            // Embed server-only data into the inputs
            for (int inputIndex = 0; inputIndex < inputs.Length; inputIndex++)
            {
                // Store the issuing player's ID to this command
                inputs[inputIndex].issuingPlayer = playerID;
            }

            // Lock to prevent race conditions
            lock (ScheduledInputs)
            {
                // Create the scheduled input array for this step if not done yet
                if (!ScheduledInputs.ContainsKey(targetInputStep))
                {
                    // Add a new container for the inputs for the current scheduled step
                    var inputsList = new List<Input>();
                
                    // Store the inputs array to the scheduled inputs list
                    ScheduledInputs.Add(targetInputStep, inputsList);
                    InputsReceived.Add(targetInputStep, 0);
                }
                
                // Store the inputs to the scheduled step
                List<Input> playerInputs = ScheduledInputs[targetInputStep];
                foreach (var input in inputs)
                {
                    // Store the input index to that player's inputs array
                    playerInputs.Add(input);
                }
                
                // Increase the number of inputs received
                InputsReceived[targetInputStep] += 1;
            
                // Proceed only if all players have submitted their states
                if (InputsReceived[targetInputStep] != playerCount) return;
                
                // ANTI-CHEAT
                // Clear authority from units which the given player does not have authority to.
                // Todo: Investigate if this chunks performance
                playerInputs.ForEach(input =>
                {
                    _issuingPlayerID = input.issuingPlayer;
                    input.selectedUnits = input.selectedUnits
                                               .Where(unit => Manager.Units[unit].team == Players[_issuingPlayerID].team)
                                               .ToArray();
                });
                
                // Store the inputs for this frame if set to record.
// Todo: Just a measure until we have a proper setting for this
#pragma warning disable CS0162 // Unreachable code detected
                // ReSharper disable once HeuristicUnreachableCode
                if (RecordInputs) recordedInputs.Add(targetInputStep, playerInputs.ToArray());
#pragma warning restore CS0162 // Unreachable code detected

                // Send the inputs out to all clients
                _PropagateInputs(step: targetInputStep, inputs: playerInputs.ToArray());
            }
        }
        /// <summary>
        /// Propagates the provided inputs to be scheduled on all clients.
        /// </summary>
        /// <param name="step">The step to schedule at.</param>
        /// <param name="inputs">The input lists of each scheduled action for each player.</param>
        [ClientRpc]
        private void _PropagateInputs(int step, Input[] inputs)
        {
            // Store the inputs to be executed
            if (isClientOnly) // Hacky catch for listen host (prevent doubling up of commands from server & client)
            {
                // Store the scheduled inputs
                ScheduledInputs.Add(step, inputs.ToList());
                // Update our logged number of inputs
                InputsReceived.Add(step, PlayerCap);
            }
        }
        
        
        // PRIVATE METHODS - TURN
        private static void _TryAdvanceTurn()
        {
            // Check if any player hasn't readied up yet
            if (Players.Values.Any(player => !player.isReadiedUp) return;

            // Reset each player's ready status
            foreach (Player player in Players.Values)
            {
                player.isReadiedUp = false;
            }
            
            // All players ready!
            // Increment turn counter
            Manager.turnCurrent += 1;
            // Call on-turn logic
            Manager.OnAdvanceTurn();
        }
    }
   
}
