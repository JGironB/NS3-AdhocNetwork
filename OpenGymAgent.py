'''
  --------------Universidad Nacional De Colombia -  Sede bogota---------
  ------  Curso Modelos estocasticos y simulacion en computacion y comunicaciones
  ------- 17 DE JUNIO DEL 2020
  ----Autores:
  -Andres Duvan Chaves Mosquera
  -Duvan Alberto Andrade
  -Ivan Rene Delgado Gonzalez
  -Juan Pablo Giron Bastidas
'''
#Importamos las librerias necesarias para usar opengym
import argparse
from ns3gym import ns3env

#Metodo que con base en la onda continua y los valores de la observacion (nodos) define las acciones a ejecutar por el agente
def calculate_cw_window(observation):
    difference = -np.diff(observation)
    maxDifference = np.argmax(difference)

    maxContinousWave = 50
    action = np.ones(shape=len(observation), dtype=np.uint32) * maxContinousWave
    action[maxDifference] = 5
    return action

#Codigo utilizado para soportar la definicion de los argumentos de la simulacion por consola
parser = argparse.ArgumentParser(description='Start simulation script on/off')
parser.add_argument('--start',
                    type=int,
                    default=1,
                    help='Start ns-3 simulation script 0/1, Default: 1')
parser.add_argument('--iterations',
                    type=int,
                    default=1,
                    help='Number of iterations, Default: 1')
args = parser.parse_args()
startSim = bool(args.start)
iterationNum = int(args.iterations)

port = 5555
simTime = 20 # seconds
stepTime = 0.5  # seconds
seed = 0
simArgs = {"--simTime": simTime,
           "--testArg": 123}
debug = False

#Definicion del ambiente (enviroment)
env = ns3env.Ns3Env(port=port, stepTime=stepTime, startSim=startSim, simSeed=seed, simArgs=simArgs, debug=debug)
# simpler:
#env = ns3env.Ns3Env()
env.reset()

#Definicion de los espacios de observacion y acciones definidos desde la simulacion de la red manet
ob_space = env.observation_space
ac_space = env.action_space
print("Observation space: ", ob_space,  ob_space.dtype)
print("Action space: ", ac_space, ac_space.dtype)

stepIdx = 0
currIt = 0


try:
    #Ciclo sobre las iteraciones que se especifican
    while True:
        print("Start iteration: ", currIt)
        obs = env.reset()
        print("Step: ", stepIdx)
        print("---obs:", obs)
        #Definicion inicial de la empresa
        while True:
            stepIdx += 1
            #Actualizacion de la recompensa utilizando el valor de los paquetes recibidos
            action = env.action_space.sample()
            print("---action: ", action)
            print("Step: ", stepIdx)
            
            #Se "da un paso" utilizando el metodo step del enviroment de gym pasandole la accion calculada previamente
            #Este metodo step retorna el vector de observacion que indica el nuevo estado en el que se encuentra el ambiente
            #Despues de realizar la accion especifica, la recompensa obtenida, el indicador de terminado e informacion extra
            obs, reward, done, info = env.step(action)
            print("---obs, reward, done, info: ", obs, reward, done, info)

            #Cuando el indicador de terminado (done) sea verdadero se dara por terminado el proceso y se reiniciaran los valores de stepIndex
            #De paquetes recividos y en caso de que sea la ultima iteracion se reiniciara el enviroment de gym
            if done:
                stepIdx = 0
                if currIt + 1 < iterationNum:
                    env.reset()
                break

        currIt += 1
        if currIt == iterationNum:
            break

except KeyboardInterrupt:
    #Soporte para la interrupcion de la simulacion utilizando dicha combinacion de teclas
    print("Ctrl-C -> Exit")
finally:
    #Cierra/ termina el ambiente de gym
    env.close()
print("Done")
