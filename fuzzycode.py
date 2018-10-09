# Fuzzy Logic with Python for detection of fire/gas leaks for an Iot cloud connected sensor systems.
# Also included SMS fuctionality to alert users and send alarm control commands to Arduino via ThingSpeak TALKBACK app.

# Technology/services used:
#-*************************-
# 1.Skfuzzy python package for fuzzy logic applications.
# 2.Nexmo - SMS API.
# 3.ThingSpeak - Cloud service to which Iot sensor data can be uploaded/downloaded.
# 4.TalkBack - A thingspeak app used to control a thingpseak connected system such as Arduino by posting commands via Talkback API

# Fuzzy System info:
#-******************-
# The automf(3 or 5 or 7) funciton automatically creates membership functions for the givrn universe range.
# The default defuzzification method is set as the 'Centroid' method.
# Input data for computation are latest posted values of temperature and gas sensor


# Packages - with their applications:
#-***********************************-
#... Numpy         - Multidimensional arrays and matrices
#... Matplotlib    - Plotting and data visualizations
#... Skfuzzy       - Fuzzy logic inference
#... Urllib        - Web URLs
#... BeautifulSoup - Parsing HTML/XML
#... Requests      - HTTP requests
#... Nexmo         - SMS API
#... Time          - Timing and delays

import numpy as np
import matplotlib.pyplot as pl
%matplotlib inline
import skfuzzy as fuzz
from skfuzzy import control as ctrl
import urllib
from bs4 import BeautifulSoup
import requests
import nexmo
import time

#----------------------Fuzzy Inference System(Mamdani)----------------------------------------

#I/p fuzzy sets
temp_sense = ctrl.Antecedent(np.arange(10, 90, 10), 'temp_sense')
gas_sense = ctrl.Antecedent(np.arange(10, 90, 10), 'gas_sense')

#O/p fuzzy set
fire_gasLeak_sense = ctrl.Consequent(np.arange(10, 90, 10), 'fire_gasLeak_sense')

#Membership functions

#temp_sense.automf(3)
#gas_sense.automf(3)
#fire_gasLeak_sense.automf(3)

temp_sense['Normal'] = fuzz.trimf(fire_gasLeak_sense.universe, [0, 10, 35])
temp_sense['Hot'] = fuzz.trapmf(fire_gasLeak_sense.universe, [25,40,50,65])
temp_sense['Critically Hot'] = fuzz.trimf(fire_gasLeak_sense.universe,[55,80,80])

gas_sense['Normal'] = fuzz.trimf(fire_gasLeak_sense.universe, [0, 10, 30])
gas_sense['High'] = fuzz.trimf(fire_gasLeak_sense.universe, [20,45, 70])
gas_sense['Explosive Leak'] = fuzz.trimf(fire_gasLeak_sense.universe, [60,80,80])

#Membership functions for fire/gasLeak fuzzy set
fire_gasLeak_sense['sec1'] = fuzz.trimf(fire_gasLeak_sense.universe, [0, 10, 45])
fire_gasLeak_sense['sec2'] = fuzz.trapmf(fire_gasLeak_sense.universe, [10, 40, 50, 80])
fire_gasLeak_sense['sec3'] = fuzz.trimf(fire_gasLeak_sense.universe,[45 , 80, 80])
#fire_gasLeak_sense['Sec4'] = fuzz.trimf(fire_gasLeak_sense.universe, [35, 45, 60])

fire_gasLeak_sense.view()
temp_sense.view()
gas_sense.view()

#RULE base
rule1 = ctrl.Rule(temp_sense['Normal'] & gas_sense['Normal'], fire_gasLeak_sense['sec1'])
rule2 = ctrl.Rule(temp_sense['Normal'] & gas_sense['High'], fire_gasLeak_sense['sec2'])
rule3 = ctrl.Rule(temp_sense['Normal'] & gas_sense['Explosive Leak'], fire_gasLeak_sense['sec3'])
rule4 = ctrl.Rule(temp_sense['Hot'] & gas_sense['Normal'], fire_gasLeak_sense['sec1'])
rule5 = ctrl.Rule(temp_sense['Hot'] & gas_sense['High'] , fire_gasLeak_sense['sec2'])
rule6 = ctrl.Rule(temp_sense['Hot'] & gas_sense['Explosive Leak'], fire_gasLeak_sense['sec3'])
rule7 = ctrl.Rule(temp_sense['Critically Hot'] & gas_sense['Normal'], fire_gasLeak_sense['sec3'])
rule8 = ctrl.Rule(temp_sense['Critically Hot'] & gas_sense['High'], fire_gasLeak_sense['sec3'])
rule9 = ctrl.Rule(temp_sense['Critically Hot'] & gas_sense['Explosive Leak'], fire_gasLeak_sense['sec3'])

#Creating a fuzzy control system using the rule base
FireGasLeak_ctrlSys = ctrl.ControlSystem([rule1, rule2, rule3, rule4, rule5 , rule6, rule7, rule8, rule9])

#Creating a simulation for our Iot systems using the above created FCS
FgcsIot = ctrl.ControlSystemSimulation(FireGasLeak_ctrlSys)

periodic_work(55)

###
def periodic_work(interval):
    while True:
        # (GET requests) to retrieve temperature and gas field values . '/last' in request indicates most recent values
        tempField_data=urllib.urlopen("https://api.thingspeak.com/channels/484808/fields/1/last?api_key=FT93ZS9HFBJCY06V&results=1");
        gasField_data=urllib.urlopen("https://api.thingspeak.com/channels/484808/fields/2/last?api_key=FT93ZS9HFBJCY06V&results=1");
        
        x=float(tempField_data.read())
        y=float(gasField_data.read())
        print("temp=",x,"  ,gas=",y)
        #Setting the i/ps for i/p fuzzy sets as the data read from the above fields
        FgcsIot.input['temp_sense']=x
        FgcsIot.input['gas_sense']=y*100 #mul by 100 to normalize

        FgcsIot.compute()
        print (FgcsIot.output['fire_gasLeak_sense'])
        fire_gasLeak_sense.view(sim=FgcsIot)
        
        #print()

        crispOP=FgcsIot.output['fire_gasLeak_sense'] #Crisp output
        #posting crisp output rounded to 2 decimal places, to channel
        time.sleep(14);
        urllib.urlopen("https://api.thingspeak.com/update?api_key=OLFQGPVYNVGQJKCX&field6="+str(round(crispOP,3)));

      # Nexmo SMS API
        client = nexmo.Client(key='Your nexmo key', secret='Your nexmo secret')
        
        if crispOP<=26.67:
            client.send_message({'from': 'Nexmo', 'to': 'Your Number', 'text': 'Normal'})
            print("01")
        if crispOP>26.67 and crispOP<=53.33:
            client.send_message({'from': 'Nexmo', 'to': 'Your Number', 'text': 'High temp/gas levels'})
            print("10")
        if crispOP>53.33:
            client.send_message({'from': 'Nexmo', 'to': 'Your Number', 'text': 'Critically High -  Gas Levels /Temperature.Immediate Action Required'})
            print("11")
        
        time.sleep(interval);
