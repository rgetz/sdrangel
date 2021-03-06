///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2019 F4EXB                                                      //
// written by Edouard Griffiths                                                  //
//                                                                               //
// This program is free software; you can redistribute it and/or modify          //
// it under the terms of the GNU General Public License as published by          //
// the Free Software Foundation as version 3 of the License, or                  //
// (at your option) any later version.                                           //
//                                                                               //
// This program is distributed in the hope that it will be useful,               //
// but WITHOUT ANY WARRANTY; without even the implied warranty of                //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                  //
// GNU General Public License V3 for more details.                               //
//                                                                               //
// You should have received a copy of the GNU General Public License             //
// along with this program. If not, see <http://www.gnu.org/licenses/>.          //
///////////////////////////////////////////////////////////////////////////////////

#ifndef SDRBASE_DSP_DEVICESAMPLEMIMO_H_
#define SDRBASE_DSP_DEVICESAMPLEMIMO_H_

#include <vector>

#include "samplesourcefifo.h"
#include "samplesinkfifo.h"
#include "util/message.h"
#include "util/messagequeue.h"
#include "export.h"

namespace SWGSDRangel
{
    class SWGDeviceSettings;
    class SWGDeviceState;
    class SWGDeviceReport;
}

class SDRBASE_API DeviceSampleMIMO : public QObject {
	Q_OBJECT
public:
    typedef enum {
        FC_POS_INFRA = 0,
        FC_POS_SUPRA,
        FC_POS_CENTER
    } fcPos_t;

	DeviceSampleMIMO();
	virtual ~DeviceSampleMIMO();
	virtual void destroy() = 0;

    virtual void init() = 0;  //!< initializations to be done when all collaborating objects are created and possibly connected
	virtual bool start() = 0;
	virtual void stop() = 0;

    virtual QByteArray serialize() const = 0;
    virtual bool deserialize(const QByteArray& data) = 0;

	virtual const QString& getDeviceDescription() const = 0;

	virtual int getSinkSampleRate(int index) const = 0;                     //!< Sample rate exposed by the sink at index
    virtual void setSinkSampleRate(int sampleRate, int index) = 0;          //!< For when the sink sample rate is set externally
	virtual quint64 getSinkCenterFrequency(int index) const = 0;            //!< Center frequency exposed by the sink at index
    virtual void setSinkCenterFrequency(qint64 centerFrequency, int index) = 0;

	virtual int getSourceSampleRate(int index) const = 0;                   //!< Sample rate exposed by the source at index
    virtual void setSourceSampleRate(int sampleRate, int index) = 0;        //!< For when the source sample rate is set externally
	virtual quint64 getSourceCenterFrequency(int index) const = 0;          //!< Center frequency exposed by the source at index
    virtual void setSourceCenterFrequency(qint64 centerFrequency, int index) = 0;

	virtual bool handleMessage(const Message& message) = 0;

    virtual int webapiSettingsGet(
            SWGSDRangel::SWGDeviceSettings& response,
            QString& errorMessage)
    {
        (void) response;
        errorMessage = "Not implemented";
        return 501;
    }

    virtual int webapiSettingsPutPatch(
            bool force, //!< true to force settings = put
            const QStringList& deviceSettingsKeys,
            SWGSDRangel::SWGDeviceSettings& response,
            QString& errorMessage)
    {
        (void) force;
        (void) deviceSettingsKeys;
        (void) response;
        errorMessage = "Not implemented";
        return 501;
    }

    virtual int webapiRunGet(
            SWGSDRangel::SWGDeviceState& response,
            QString& errorMessage)
    {
        (void) response;
        errorMessage = "Not implemented";
        return 501;
    }

    virtual int webapiRun(bool run,
            SWGSDRangel::SWGDeviceState& response,
            QString& errorMessage)
    {
        (void) run;
        (void) response;
        errorMessage = "Not implemented";
        return 501;
    }

    virtual int webapiReportGet(
            SWGSDRangel::SWGDeviceReport& response,
            QString& errorMessage)
    {
        (void) response;
        errorMessage = "Not implemented";
        return 501;
    }

	MessageQueue *getInputMessageQueue() { return &m_inputMessageQueue; }
    virtual void setMessageQueueToGUI(MessageQueue *queue) = 0; // pure virtual so that child classes must have to deal with this
    MessageQueue *getMessageQueueToGUI() { return m_guiMessageQueue; }

    unsigned int getNbSourceFifos() const { return m_sampleSourceFifos.size(); } //!< Get the number of Tx FIFOs
    unsigned int getNbSinkFifos() const { return m_sampleSinkFifos.size(); }     //!< Get the number of Rx FIFOs
	SampleSourceFifo* getSampleSourceFifo(unsigned int index); //!< Get Tx FIFO at index
    SampleSinkFifo* getSampleSinkFifo(unsigned int index);     //!< Get Rx FIFO at index
    // Streams and FIFOs are in opposed source/sink type whick makes it confusing when stream direction is involved:
    //   Rx: source stream -> sink FIFO    -> channel sinks
    //   Tx: sink stream   <- source FIFO  <- channel sources
    unsigned int getNbSourceStreams() const { return m_sampleSinkFifos.size(); } //!< Commodity function same as getNbSinkFifos (Rx or source streams)
    unsigned int getNbSinkStreams() const { return m_sampleSourceFifos.size(); } //!< Commodity function same as getNbSourceFifos (Tx or sink streams)

protected slots:
	void handleInputMessages();

protected:
    std::vector<SampleSourceFifo> m_sampleSourceFifos; //!< Tx FIFOs
    std::vector<SampleSinkFifo> m_sampleSinkFifos;     //!< Rx FIFOs
	MessageQueue m_inputMessageQueue; //!< Input queue to the sink
    MessageQueue *m_guiMessageQueue;  //!< Input message queue to the GUI
};

#endif // SDRBASE_DSP_DEVICESAMPLEMIMO_H_