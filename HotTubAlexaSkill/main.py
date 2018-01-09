import sys
sys.path.insert(0, './lib/')
from Adafruit_IO import Client

CONFIG = {
    'APP_ID': 'amzn1.ask.skill.YOUR_AMAZON_SKILL_ID',
    'adafruitIO_key': 'YOUR_ADAFRUIT_IO_KEY',
    'adafruitIO_feed': 'YOUR_ADAFRUIT_FEED'

	
	
	
}


def lambda_handler(event, context):

    if (event['session']['application']['applicationId'] != CONFIG['APP_ID']):
      raise AuthorizationError

    if (event['request']['intent']['name'] == "TurnOff"):
        spa_control('POOL')
        return generateJSON('I turned off the spa', 'Spa was turned off.')

    elif(event['request']['intent']['name'] == "TurnOn"):
        spa_control('SPA')
        return generateJSON('I turned on the spa', 'Spa was turned on.')

    elif(event['request']['intent']['name'] == "CheckStatus"):
        state = spa_state()
        return generateJSON('The spa is '+ state, 'Spa is currently '+ state)
		
    elif(event['request']['intent']['name'] == "CheckTemp"):
        temp = spa_temp()
        return generateJSON('The spa is '+ temp +'degrees', 'Spa is currently '+ temp +'degrees')

    else:
        raise NamespaceError


def spa_control(state):
    io = Client(CONFIG['adafruitIO_key'])
    io.send(CONFIG['adafruitIO_feed'], state)


def spa_state():
    io = Client(CONFIG['adafruitIO_key'])
    return 'on' if io.receive('Current_Temperature').value == 'SPA' else 'off'

def spa_temp():
	io = Client(CONFIG['adafruitIO_key'])
	return io.receive(CONFIG['adafruitIO_feed2']).value
	

def generateJSON(speechText, cardText):
    return {
      "version": "1.0",
      "response": {
        "outputSpeech": {
          "type": "PlainText",
          "text": speechText
        },
        "card": {
          "content": cardText,
          "title": "Spa Controller",
          "type": "Simple"
        },
        "shouldEndSession": True
      },
      "sessionAttributes": {}
    }
