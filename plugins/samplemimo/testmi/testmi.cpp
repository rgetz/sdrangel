///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2019 Edouard Griffiths, F4EXB                                   //
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

#include <string.h>
#include <errno.h>

#include <QDebug>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QBuffer>

#include "SWGDeviceSettings.h"
#include "SWGDeviceState.h"
#include "SWGTestMISettings.h"

#include "device/deviceapi.h"
#include "dsp/dspcommands.h"
#include "dsp/dspengine.h"
#include "dsp/dspdevicemimoengine.h"
#include "dsp/devicesamplesource.h"
#include "dsp/filerecord.h"

#include "testmithread.h"
#include "testmi.h"

MESSAGE_CLASS_DEFINITION(TestMI::MsgConfigureTestSource, Message)
MESSAGE_CLASS_DEFINITION(TestMI::MsgFileRecord, Message)
MESSAGE_CLASS_DEFINITION(TestMI::MsgStartStop, Message)


TestMI::TestMI(DeviceAPI *deviceAPI) :
    m_deviceAPI(deviceAPI),
	m_settings(),
	m_deviceDescription(),
	m_running(false),
	m_masterTimer(deviceAPI->getMasterTimer())
{
    m_deviceAPI->setNbSourceStreams(2);
    m_deviceAPI->addSourceStream(true); // Add a new source stream data set in the engine - asynchronous handling of FIFOs
    m_deviceAPI->addSourceStream(true); // Add a new source stream data set in the engine - asynchronous handling of FIFOs
    m_sampleSinkFifos.push_back(SampleSinkFifo(96000 * 4));
    m_sampleSinkFifos.push_back(SampleSinkFifo(96000 * 4));
    m_networkManager = new QNetworkAccessManager();
    connect(m_networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(networkManagerFinished(QNetworkReply*)));
}

TestMI::~TestMI()
{
    disconnect(m_networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(networkManagerFinished(QNetworkReply*)));
    delete m_networkManager;

    if (m_running) {
        stop();
    }

    std::vector<FileRecord*>::iterator it = m_fileSinks.begin();
    int istream = 0;

    for (; it != m_fileSinks.end(); ++it, istream++)
    {
        m_deviceAPI->removeAncillarySink(*it, istream);
        delete *it;
    }

    m_deviceAPI->removeLastSourceStream(); // Remove the last source stream data set in the engine
    m_deviceAPI->removeLastSourceStream(); // Remove the last source stream data set in the engine
}

void TestMI::destroy()
{
    delete this;
}

void TestMI::init()
{
    m_fileSinks.push_back(new FileRecord(QString("test_0_%1.sdriq").arg(m_deviceAPI->getDeviceUID())));
    m_fileSinks.push_back(new FileRecord(QString("test_1_%1.sdriq").arg(m_deviceAPI->getDeviceUID())));
    m_deviceAPI->addAncillarySink(m_fileSinks[0], 0);
    m_deviceAPI->addAncillarySink(m_fileSinks[1], 1);

    applySettings(m_settings, true);
}

bool TestMI::start()
{
    qDebug("TestMI::start");
	QMutexLocker mutexLocker(&m_mutex);

    if (m_running) {
        stop();
    }

    m_testSourceThreads.push_back(new TestMIThread(&m_sampleSinkFifos[0], 0));
	m_testSourceThreads.back()->setSamplerate(m_settings.m_streams[0].m_sampleRate);
	m_testSourceThreads.back()->startStop(true);

    m_testSourceThreads.push_back(new TestMIThread(&m_sampleSinkFifos[1], 1));
	m_testSourceThreads.back()->setSamplerate(m_settings.m_streams[1].m_sampleRate);
	m_testSourceThreads.back()->startStop(true);

	mutexLocker.unlock();

	applySettings(m_settings, true);
	m_running = true;

	return true;
}

void TestMI::stop()
{
    qDebug("TestMI::stop");
	QMutexLocker mutexLocker(&m_mutex);

    std::vector<TestMIThread*>::iterator it = m_testSourceThreads.begin();

    for (; it != m_testSourceThreads.end(); ++it)
    {
        (*it)->startStop(false);
        (*it)->deleteLater();
    }

    m_testSourceThreads.clear();
	m_running = false;
}

QByteArray TestMI::serialize() const
{
    return m_settings.serialize();
}

bool TestMI::deserialize(const QByteArray& data)
{
    bool success = true;

    if (!m_settings.deserialize(data))
    {
        m_settings.resetToDefaults();
        success = false;
    }

    MsgConfigureTestSource* message = MsgConfigureTestSource::create(m_settings, true);
    m_inputMessageQueue.push(message);

    if (m_guiMessageQueue)
    {
        MsgConfigureTestSource* messageToGUI = MsgConfigureTestSource::create(m_settings, true);
        m_guiMessageQueue->push(messageToGUI);
    }

    return success;
}

const QString& TestMI::getDeviceDescription() const
{
	return m_deviceDescription;
}

int TestMI::getSourceSampleRate(int index) const
{
    if (index < (int) m_settings.m_streams.size()) {
	    return m_settings.m_streams[index].m_sampleRate/(1<<m_settings.m_streams[index].m_log2Decim);
    } else {
        return 0;
    }
}

quint64 TestMI::getSourceCenterFrequency(int index) const
{
    if (index < (int) m_settings.m_streams.size()) {
    	return m_settings.m_streams[index].m_centerFrequency;
    } else {
        return 0;
    }
}

void TestMI::setSourceCenterFrequency(qint64 centerFrequency, int index)
{
    TestMISettings settings = m_settings; // note: calls copy constructor

    if (index < (int) settings.m_streams.size())
    {
        settings.m_streams[index].m_centerFrequency = centerFrequency;

        MsgConfigureTestSource* message = MsgConfigureTestSource::create(settings, false);
        m_inputMessageQueue.push(message);

        if (m_guiMessageQueue)
        {
            MsgConfigureTestSource* messageToGUI = MsgConfigureTestSource::create(settings, false);
            m_guiMessageQueue->push(messageToGUI);
        }
    }
}

bool TestMI::handleMessage(const Message& message)
{
    if (MsgConfigureTestSource::match(message))
    {
        MsgConfigureTestSource& conf = (MsgConfigureTestSource&) message;
        qDebug() << "TestMI::handleMessage: MsgConfigureTestSource";

        bool success = applySettings(conf.getSettings(), conf.getForce());

        if (!success)
        {
            qDebug("TestMI::handleMessage: config error");
        }

        return true;
    }
    else if (MsgFileRecord::match(message))
    {
        MsgFileRecord& conf = (MsgFileRecord&) message;
        qDebug() << "TestMI::handleMessage: MsgFileRecord: " << conf.getStartStop();
        int istream = conf.getStreamIndex();

        if (conf.getStartStop())
        {
            if (m_settings.m_fileRecordName.size() != 0) {
                m_fileSinks[istream]->setFileName(m_settings.m_fileRecordName + "_0.sdriq");
            } else {
                m_fileSinks[istream]->genUniqueFileName(m_deviceAPI->getDeviceUID(), istream);
            }

            m_fileSinks[istream]->startRecording();
        }
        else
        {
            m_fileSinks[istream]->stopRecording();
        }

        return true;
    }
    else if (MsgStartStop::match(message))
    {
        MsgStartStop& cmd = (MsgStartStop&) message;
        qDebug() << "TestMI::handleMessage: MsgStartStop: " << (cmd.getStartStop() ? "start" : "stop");

        if (cmd.getStartStop())
        {
            if (m_deviceAPI->initDeviceEngine())
            {
                m_deviceAPI->startDeviceEngine();
            }
        }
        else
        {
            m_deviceAPI->stopDeviceEngine();
        }

        if (m_settings.m_useReverseAPI) {
            webapiReverseSendStartStop(cmd.getStartStop());
        }

        return true;
    }
    else
    {
        return false;
    }
}

bool TestMI::applySettings(const TestMISettings& settings, bool force)
{
    DeviceSettingsKeys deviceSettingsKeys;

    qDebug() << "TestMI::applySettings: common: "
        << " m_fileRecordName: " << settings.m_fileRecordName
        << " m_useReverseAPI: " << settings.m_useReverseAPI
        << " m_reverseAPIAddress: " << settings.m_reverseAPIAddress
        << " m_reverseAPIPort: " << settings.m_reverseAPIPort
        << " m_reverseAPIDeviceIndex: " << settings.m_reverseAPIDeviceIndex;

    for (unsigned int istream = 0; (istream < m_settings.m_streams.size()) && (istream < settings.m_streams.size()); istream++)
    {
        qDebug() << "TestMI::applySettings: stream #" << istream << ": "
            << " m_centerFrequency: " << settings.m_streams[istream].m_centerFrequency
            << " m_frequencyShift: " << settings.m_streams[istream].m_frequencyShift
            << " m_sampleRate: " << settings.m_streams[istream].m_sampleRate
            << " m_log2Decim: " << settings.m_streams[istream].m_log2Decim
            << " m_fcPos: " << settings.m_streams[istream].m_fcPos
            << " m_amplitudeBits: " << settings.m_streams[istream].m_amplitudeBits
            << " m_sampleSizeIndex: " << settings.m_streams[istream].m_sampleSizeIndex
            << " m_autoCorrOptions: " << settings.m_streams[istream].m_autoCorrOptions
            << " m_dcFactor: " << settings.m_streams[istream].m_dcFactor
            << " m_iFactor: " << settings.m_streams[istream].m_iFactor
            << " m_qFactor: " << settings.m_streams[istream].m_qFactor
            << " m_phaseImbalance: " << settings.m_streams[istream].m_phaseImbalance
            << " m_modulation: " << settings.m_streams[istream].m_modulation
            << " m_amModulation: " << settings.m_streams[istream].m_amModulation
            << " m_fmDeviation: " << settings.m_streams[istream].m_fmDeviation
            << " m_modulationTone: " << settings.m_streams[istream].m_modulationTone;

        deviceSettingsKeys.m_streamsSettingsKeys.push_back(QList<QString>());
        QList<QString>& reverseAPIKeys = deviceSettingsKeys.m_streamsSettingsKeys.back();

        if ((m_settings.m_streams[istream].m_autoCorrOptions != settings.m_streams[istream].m_autoCorrOptions) || force)
        {
            reverseAPIKeys.append("autoCorrOptions");

            switch(settings.m_streams[istream].m_autoCorrOptions)
            {
            case TestMIStreamSettings::AutoCorrDC:
                m_deviceAPI->configureCorrections(true, false, istream);
                break;
            case TestMIStreamSettings::AutoCorrDCAndIQ:
                m_deviceAPI->configureCorrections(true, true, istream);
                break;
            case TestMIStreamSettings::AutoCorrNone:
            default:
                m_deviceAPI->configureCorrections(false, false, istream);
                break;
            }
        }

        if ((m_settings.m_streams[istream].m_sampleRate != settings.m_streams[istream].m_sampleRate) || force)
        {
            reverseAPIKeys.append("sampleRate");

            if ((istream < m_testSourceThreads.size()) && (m_testSourceThreads[istream]))
            {
                m_testSourceThreads[istream]->setSamplerate(settings.m_streams[istream].m_sampleRate);
                qDebug("TestMI::applySettings: thread on stream: %u sample rate set to %d",
                    istream, settings.m_streams[istream].m_sampleRate);
            }
        }

        if ((m_settings.m_streams[istream].m_log2Decim != settings.m_streams[istream].m_log2Decim) || force)
        {
            reverseAPIKeys.append("log2Decim");

            if ((istream < m_testSourceThreads.size()) && (m_testSourceThreads[istream]))
            {
                m_testSourceThreads[istream]->setLog2Decimation(settings.m_streams[istream].m_log2Decim);
                qDebug("TestMI::applySettings: thread on stream: %u set decimation to %d",
                    istream, (1<<settings.m_streams[istream].m_log2Decim));
            }
        }

        if ((m_settings.m_streams[istream].m_centerFrequency != settings.m_streams[istream].m_centerFrequency)
            || (m_settings.m_streams[istream].m_fcPos != settings.m_streams[istream].m_fcPos)
            || (m_settings.m_streams[istream].m_frequencyShift != settings.m_streams[istream].m_frequencyShift)
            || (m_settings.m_streams[istream].m_sampleRate != settings.m_streams[istream].m_sampleRate)
            || (m_settings.m_streams[istream].m_log2Decim != settings.m_streams[istream].m_log2Decim) || force)
        {
            reverseAPIKeys.append("centerFrequency");
            reverseAPIKeys.append("fcPos");
            reverseAPIKeys.append("frequencyShift");

            qint64 deviceCenterFrequency = DeviceSampleSource::calculateDeviceCenterFrequency(
                    settings.m_streams[istream].m_centerFrequency,
                    0, // no transverter mode
                    settings.m_streams[istream].m_log2Decim,
                    (DeviceSampleSource::fcPos_t) settings.m_streams[istream].m_fcPos,
                    settings.m_streams[istream].m_sampleRate,
                    DeviceSampleSource::FrequencyShiftScheme::FSHIFT_STD,
                    false);

            int frequencyShift = settings.m_streams[istream].m_frequencyShift;
            quint32 devSampleRate = settings.m_streams[istream].m_sampleRate;

            if (settings.m_streams[istream].m_log2Decim != 0)
            {
                frequencyShift += DeviceSampleSource::calculateFrequencyShift(
                        settings.m_streams[istream].m_log2Decim,
                        (DeviceSampleSource::fcPos_t) settings.m_streams[istream].m_fcPos,
                        settings.m_streams[istream].m_sampleRate,
                        DeviceSampleSource::FSHIFT_STD);
            }

            if ((istream < m_testSourceThreads.size()) && (m_testSourceThreads[istream]))
            {
                m_testSourceThreads[istream]->setFcPos((int) settings.m_streams[istream].m_fcPos);
                m_testSourceThreads[istream]->setFrequencyShift(frequencyShift);
                qDebug() << "TestMI::applySettings:"
                        << " thread on istream: " << istream
                        << " center freq: " << settings.m_streams[istream].m_centerFrequency << " Hz"
                        << " device center freq: " << deviceCenterFrequency << " Hz"
                        << " device sample rate: " << devSampleRate << "Hz"
                        << " Actual sample rate: " << devSampleRate/(1<<m_settings.m_streams[istream].m_log2Decim) << "Hz"
                        << " f shift: " << settings.m_streams[istream].m_frequencyShift;
            }
        }

        if ((m_settings.m_streams[istream].m_amplitudeBits != settings.m_streams[istream].m_amplitudeBits) || force)
        {
            reverseAPIKeys.append("amplitudeBits");

            if ((istream < m_testSourceThreads.size()) && (m_testSourceThreads[istream])) {
                m_testSourceThreads[istream]->setAmplitudeBits(settings.m_streams[istream].m_amplitudeBits);
            }
        }

        if ((m_settings.m_streams[istream].m_dcFactor != settings.m_streams[istream].m_dcFactor) || force)
        {
            reverseAPIKeys.append("dcFactor");

            if ((istream < m_testSourceThreads.size()) && (m_testSourceThreads[istream])) {
                m_testSourceThreads[istream]->setDCFactor(settings.m_streams[istream].m_dcFactor);
            }
        }

        if ((m_settings.m_streams[istream].m_iFactor != settings.m_streams[istream].m_iFactor) || force)
        {
            reverseAPIKeys.append("iFactor");

            if ((istream < m_testSourceThreads.size()) && (m_testSourceThreads[istream])) {
                m_testSourceThreads[istream]->setIFactor(settings.m_streams[istream].m_iFactor);
            }
        }

        if ((m_settings.m_streams[istream].m_qFactor != settings.m_streams[istream].m_qFactor) || force)
        {
            reverseAPIKeys.append("qFactor");

            if ((istream < m_testSourceThreads.size()) && (m_testSourceThreads[istream])) {
                m_testSourceThreads[istream]->setQFactor(settings.m_streams[istream].m_qFactor);
            }
        }

        if ((m_settings.m_streams[istream].m_phaseImbalance != settings.m_streams[istream].m_phaseImbalance) || force)
        {
            reverseAPIKeys.append("phaseImbalance");

            if ((istream < m_testSourceThreads.size()) && (m_testSourceThreads[istream])) {
                m_testSourceThreads[istream]->setPhaseImbalance(settings.m_streams[istream].m_phaseImbalance);
            }
        }

        if ((m_settings.m_streams[istream].m_sampleSizeIndex != settings.m_streams[istream].m_sampleSizeIndex) || force)
        {
            reverseAPIKeys.append("sampleSizeIndex");

            if ((istream < m_testSourceThreads.size()) && (m_testSourceThreads[istream])) {
                m_testSourceThreads[istream]->setBitSize(settings.m_streams[istream].m_sampleSizeIndex);
            }
        }

        if ((m_settings.m_streams[istream].m_sampleRate != settings.m_streams[istream].m_sampleRate)
            || (m_settings.m_streams[istream].m_centerFrequency != settings.m_streams[istream].m_centerFrequency)
            || (m_settings.m_streams[istream].m_log2Decim != settings.m_streams[istream].m_log2Decim)
            || (m_settings.m_streams[istream].m_fcPos != settings.m_streams[istream].m_fcPos) || force)
        {
            int sampleRate = settings.m_streams[istream].m_sampleRate/(1<<settings.m_streams[istream].m_log2Decim);
            DSPSignalNotification notif(sampleRate, settings.m_streams[istream].m_centerFrequency);
            m_fileSinks[istream]->handleMessage(notif); // forward to file sink
            DSPMIMOSignalNotification *engineNotif = new DSPMIMOSignalNotification(
                sampleRate, settings.m_streams[istream].m_centerFrequency, true, istream);
            m_deviceAPI->getDeviceEngineInputMessageQueue()->push(engineNotif);
        }

        if ((m_settings.m_streams[istream].m_modulationTone != settings.m_streams[istream].m_modulationTone) || force)
        {
            reverseAPIKeys.append("modulationTone");

            if ((istream < m_testSourceThreads.size()) && (m_testSourceThreads[istream])) {
                m_testSourceThreads[istream]->setToneFrequency(settings.m_streams[istream].m_modulationTone * 10);
            }
        }

        if ((m_settings.m_streams[istream].m_modulation != settings.m_streams[istream].m_modulation) || force)
        {
            reverseAPIKeys.append("modulation");

            if ((istream < m_testSourceThreads.size()) && (m_testSourceThreads[istream]))
            {
                m_testSourceThreads[istream]->setModulation(settings.m_streams[istream].m_modulation);

                if (settings.m_streams[istream].m_modulation == TestMIStreamSettings::ModulationPattern0) {
                    m_testSourceThreads[istream]->setPattern0();
                } else if (settings.m_streams[istream].m_modulation == TestMIStreamSettings::ModulationPattern1) {
                    m_testSourceThreads[istream]->setPattern1();
                } else if (settings.m_streams[istream].m_modulation == TestMIStreamSettings::ModulationPattern2) {
                    m_testSourceThreads[istream]->setPattern2();
                }
            }
        }

        if ((m_settings.m_streams[istream].m_amModulation != settings.m_streams[istream].m_amModulation) || force)
        {
            reverseAPIKeys.append("amModulation");

            if ((istream < m_testSourceThreads.size()) && (m_testSourceThreads[istream])) {
                m_testSourceThreads[istream]->setAMModulation(settings.m_streams[istream].m_amModulation / 100.0f);
            }
        }

        if ((m_settings.m_streams[istream].m_fmDeviation != settings.m_streams[istream].m_fmDeviation) || force)
        {
            reverseAPIKeys.append("fmDeviation");

            if ((istream < m_testSourceThreads.size()) && (m_testSourceThreads[istream])) {
                m_testSourceThreads[istream]->setFMDeviation(settings.m_streams[istream].m_fmDeviation * 100.0f);
            }
        }
    } // for each stream index

    if (settings.m_useReverseAPI)
    {
        qDebug("TestMI::applySettings: call webapiReverseSendSettings");
        bool fullUpdate = ((m_settings.m_useReverseAPI != settings.m_useReverseAPI) && settings.m_useReverseAPI) ||
                (m_settings.m_reverseAPIAddress != settings.m_reverseAPIAddress) ||
                (m_settings.m_reverseAPIPort != settings.m_reverseAPIPort) ||
                (m_settings.m_reverseAPIDeviceIndex != settings.m_reverseAPIDeviceIndex);
        webapiReverseSendSettings(deviceSettingsKeys, settings, fullUpdate || force);
    }

    m_settings = settings;
    return true;
}

int TestMI::webapiRunGet(
        SWGSDRangel::SWGDeviceState& response,
        QString& errorMessage)
{
    (void) errorMessage;
    m_deviceAPI->getDeviceEngineStateStr(*response.getState());
    return 200;
}

int TestMI::webapiRun(
        bool run,
        SWGSDRangel::SWGDeviceState& response,
        QString& errorMessage)
{
    (void) errorMessage;
    m_deviceAPI->getDeviceEngineStateStr(*response.getState());
    MsgStartStop *message = MsgStartStop::create(run);
    m_inputMessageQueue.push(message);

    if (m_guiMessageQueue) // forward to GUI if any
    {
        MsgStartStop *msgToGUI = MsgStartStop::create(run);
        m_guiMessageQueue->push(msgToGUI);
    }

    return 200;
}

int TestMI::webapiSettingsGet(
                SWGSDRangel::SWGDeviceSettings& response,
                QString& errorMessage)
{
    (void) errorMessage;
    response.setTestMiSettings(new SWGSDRangel::SWGTestMISettings());
    response.getTestMiSettings()->init();
    webapiFormatDeviceSettings(response, m_settings);
    return 200;
}

int TestMI::webapiSettingsPutPatch(
                bool force,
                const QStringList& deviceSettingsKeys,
                SWGSDRangel::SWGDeviceSettings& response, // query + response
                QString& errorMessage)
{
    (void) errorMessage;
    TestMISettings settings = m_settings;

    if (deviceSettingsKeys.contains("streams"))
    {
        QList<SWGSDRangel::SWGTestMiStreamSettings*> *streamsSettings = response.getTestMiSettings()->getStreams();
        QList<SWGSDRangel::SWGTestMiStreamSettings*>::const_iterator it = streamsSettings->begin();

        for (; it != streamsSettings->end(); ++it)
        {
            int istream = (*it)->getStreamIndex();

            if (deviceSettingsKeys.contains(tr("streams[%1].centerFrequency").arg(istream))) {
                settings.m_streams[istream].m_centerFrequency = (*it)->getCenterFrequency();
            }
            if (deviceSettingsKeys.contains(tr("streams[%1].frequencyShift").arg(istream))) {
                settings.m_streams[istream].m_frequencyShift = (*it)->getFrequencyShift();
            }
            if (deviceSettingsKeys.contains(tr("streams[%1].sampleRate").arg(istream))) {
                settings.m_streams[istream].m_sampleRate = (*it)->getSampleRate();
            }
            if (deviceSettingsKeys.contains(tr("streams[%1].log2Decim").arg(istream))) {
                settings.m_streams[istream].m_log2Decim = (*it)->getLog2Decim();
            }
            if (deviceSettingsKeys.contains(tr("streams[%1].fcPos").arg(istream))) {
                int fcPos = (*it)->getFcPos();
                fcPos = fcPos < 0 ? 0 : fcPos > 2 ? 2 : fcPos;
                settings.m_streams[istream].m_fcPos = (TestMIStreamSettings::fcPos_t) fcPos;
            }
            if (deviceSettingsKeys.contains(tr("streams[%1].sampleSizeIndex").arg(istream))) {
                int sampleSizeIndex = (*it)->getSampleSizeIndex();
                sampleSizeIndex = sampleSizeIndex < 0 ? 0 : sampleSizeIndex > 1 ? 2 : sampleSizeIndex;
                settings.m_streams[istream].m_sampleSizeIndex = sampleSizeIndex;
            }
            if (deviceSettingsKeys.contains(tr("streams[%1].amplitudeBits").arg(istream))) {
                settings.m_streams[istream].m_amplitudeBits = (*it)->getAmplitudeBits();
            }
            if (deviceSettingsKeys.contains(tr("streams[%1].autoCorrOptions").arg(istream))) {
                int autoCorrOptions = (*it)->getAutoCorrOptions();
                autoCorrOptions = autoCorrOptions < 0 ? 0 : autoCorrOptions >= TestMIStreamSettings::AutoCorrLast ? TestMIStreamSettings::AutoCorrLast-1 : autoCorrOptions;
                settings.m_streams[istream].m_sampleSizeIndex = (TestMIStreamSettings::AutoCorrOptions) autoCorrOptions;
            }
            if (deviceSettingsKeys.contains(tr("streams[%1].modulation").arg(istream))) {
                int modulation = (*it)->getModulation();
                modulation = modulation < 0 ? 0 : modulation >= TestMIStreamSettings::ModulationLast ? TestMIStreamSettings::ModulationLast-1 : modulation;
                settings.m_streams[istream].m_modulation = (TestMIStreamSettings::Modulation) modulation;
            }
            if (deviceSettingsKeys.contains(tr("streams[%1].modulationTone").arg(istream))) {
                settings.m_streams[istream].m_modulationTone = (*it)->getModulationTone();
            }
            if (deviceSettingsKeys.contains(tr("streams[%1].amModulation").arg(istream))) {
                settings.m_streams[istream].m_amModulation = (*it)->getAmModulation();
            };
            if (deviceSettingsKeys.contains(tr("streams[%1].fmDeviation").arg(istream))) {
                settings.m_streams[istream].m_fmDeviation = (*it)->getFmDeviation();
            };
            if (deviceSettingsKeys.contains(tr("streams[%1].dcFactor").arg(istream))) {
                settings.m_streams[istream].m_dcFactor = (*it)->getDcFactor();
            };
            if (deviceSettingsKeys.contains(tr("streams[%1].iFactor").arg(istream))) {
                settings.m_streams[istream].m_iFactor = (*it)->getIFactor();
            };
            if (deviceSettingsKeys.contains(tr("streams[%1].qFactor").arg(istream))) {
                settings.m_streams[istream].m_qFactor = (*it)->getQFactor();
            };
            if (deviceSettingsKeys.contains(tr("streams[%1].phaseImbalance").arg(istream))) {
                settings.m_streams[istream].m_phaseImbalance = (*it)->getPhaseImbalance();
            };
        }

    }

    if (deviceSettingsKeys.contains("fileRecordName")) {
        settings.m_fileRecordName = *response.getTestMiSettings()->getFileRecordName();
    }
    if (deviceSettingsKeys.contains("useReverseAPI")) {
        settings.m_useReverseAPI = response.getTestMiSettings()->getUseReverseApi() != 0;
    }
    if (deviceSettingsKeys.contains("reverseAPIAddress")) {
        settings.m_reverseAPIAddress = *response.getTestMiSettings()->getReverseApiAddress();
    }
    if (deviceSettingsKeys.contains("reverseAPIPort")) {
        settings.m_reverseAPIPort = response.getTestMiSettings()->getReverseApiPort();
    }
    if (deviceSettingsKeys.contains("reverseAPIDeviceIndex")) {
        settings.m_reverseAPIDeviceIndex = response.getTestMiSettings()->getReverseApiDeviceIndex();
    }

    MsgConfigureTestSource *msg = MsgConfigureTestSource::create(settings, force);
    m_inputMessageQueue.push(msg);

    if (m_guiMessageQueue) // forward to GUI if any
    {
        MsgConfigureTestSource *msgToGUI = MsgConfigureTestSource::create(settings, force);
        m_guiMessageQueue->push(msgToGUI);
    }

    webapiFormatDeviceSettings(response, settings);
    return 200;
}

void TestMI::webapiFormatDeviceSettings(SWGSDRangel::SWGDeviceSettings& response, const TestMISettings& settings)
{
    std::vector<TestMIStreamSettings>::const_iterator it = settings.m_streams.begin();
    int istream = 0;

    for (; it != settings.m_streams.end(); ++it, istream++)
    {
        QList<SWGSDRangel::SWGTestMiStreamSettings*> *streams = response.getTestMiSettings()->getStreams();
        streams->append(new SWGSDRangel::SWGTestMiStreamSettings);
        streams->back()->init();
        streams->back()->setStreamIndex(istream);
        streams->back()->setCenterFrequency(it->m_centerFrequency);
        streams->back()->setFrequencyShift(it->m_frequencyShift);
        streams->back()->setSampleRate(it->m_sampleRate);
        streams->back()->setLog2Decim(it->m_log2Decim);
        streams->back()->setFcPos((int) it->m_fcPos);
        streams->back()->setSampleSizeIndex((int) it->m_sampleSizeIndex);
        streams->back()->setAmplitudeBits(it->m_amplitudeBits);
        streams->back()->setAutoCorrOptions((int) it->m_autoCorrOptions);
        streams->back()->setModulation((int) it->m_modulation);
        streams->back()->setModulationTone(it->m_modulationTone);
        streams->back()->setAmModulation(it->m_amModulation);
        streams->back()->setFmDeviation(it->m_fmDeviation);
        streams->back()->setDcFactor(it->m_dcFactor);
        streams->back()->setIFactor(it->m_iFactor);
        streams->back()->setQFactor(it->m_qFactor);
        streams->back()->setPhaseImbalance(it->m_phaseImbalance);
    }

    if (response.getTestMiSettings()->getFileRecordName()) {
        *response.getTestMiSettings()->getFileRecordName() = settings.m_fileRecordName;
    } else {
        response.getTestMiSettings()->setFileRecordName(new QString(settings.m_fileRecordName));
    }

    response.getTestMiSettings()->setUseReverseApi(settings.m_useReverseAPI ? 1 : 0);

    if (response.getTestMiSettings()->getReverseApiAddress()) {
        *response.getTestMiSettings()->getReverseApiAddress() = settings.m_reverseAPIAddress;
    } else {
        response.getTestMiSettings()->setReverseApiAddress(new QString(settings.m_reverseAPIAddress));
    }

    response.getTestMiSettings()->setReverseApiPort(settings.m_reverseAPIPort);
    response.getTestMiSettings()->setReverseApiDeviceIndex(settings.m_reverseAPIDeviceIndex);
}

void TestMI::webapiReverseSendSettings(const DeviceSettingsKeys& deviceSettingsKeys, const TestMISettings& settings, bool force)
{
    SWGSDRangel::SWGDeviceSettings *swgDeviceSettings = new SWGSDRangel::SWGDeviceSettings();
    swgDeviceSettings->setDirection(0); // single Rx
    swgDeviceSettings->setOriginatorIndex(m_deviceAPI->getDeviceSetIndex());
    swgDeviceSettings->setDeviceHwType(new QString("TestSource"));
    swgDeviceSettings->setTestMiSettings(new SWGSDRangel::SWGTestMISettings());
    SWGSDRangel::SWGTestMISettings *swgTestMISettings = swgDeviceSettings->getTestMiSettings();

    // transfer data that has been modified. When force is on transfer all data except reverse API data

    QList<QList<QString>>::const_iterator it = deviceSettingsKeys.m_streamsSettingsKeys.begin();
    int istream = 0;

    for (; it != deviceSettingsKeys.m_streamsSettingsKeys.end(); ++it, istream++)
    {
        if ((it->size() > 0) || force)
        {
            QList<SWGSDRangel::SWGTestMiStreamSettings*> *streams = swgTestMISettings->getStreams();
            streams->append(new SWGSDRangel::SWGTestMiStreamSettings);
            streams->back()->init();
            streams->back()->setStreamIndex(istream);
            const QList<QString>& streamSettingsKeys = *it;

            if (streamSettingsKeys.contains("centerFrequency") || force) {
                streams->back()->setCenterFrequency(settings.m_streams[istream].m_centerFrequency);
            }
            if (streamSettingsKeys.contains("frequencyShift") || force) {
                streams->back()->setFrequencyShift(settings.m_streams[istream].m_frequencyShift);
            }
            if (streamSettingsKeys.contains("sampleRate") || force) {
                streams->back()->setSampleRate(settings.m_streams[istream].m_sampleRate);
            }
            if (streamSettingsKeys.contains("log2Decim") || force) {
                streams->back()->setLog2Decim(settings.m_streams[istream].m_log2Decim);
            }
            if (streamSettingsKeys.contains("fcPos") || force) {
                streams->back()->setFcPos((int) settings.m_streams[istream].m_fcPos);
            }
            if (streamSettingsKeys.contains("sampleSizeIndex") || force) {
                streams->back()->setSampleSizeIndex(settings.m_streams[istream].m_sampleSizeIndex);
            }
            if (streamSettingsKeys.contains("amplitudeBits") || force) {
                streams->back()->setAmplitudeBits(settings.m_streams[istream].m_amplitudeBits);
            }
            if (streamSettingsKeys.contains("autoCorrOptions") || force) {
                streams->back()->setAutoCorrOptions((int) settings.m_streams[istream].m_sampleSizeIndex);
            }
            if (streamSettingsKeys.contains("modulation") || force) {
                streams->back()->setModulation((int) settings.m_streams[istream].m_modulation);
            }
            if (streamSettingsKeys.contains("modulationTone")) {
                streams->back()->setModulationTone(settings.m_streams[istream].m_modulationTone);
            }
            if (streamSettingsKeys.contains("amModulation") || force) {
                streams->back()->setAmModulation(settings.m_streams[istream].m_amModulation);
            };
            if (streamSettingsKeys.contains("fmDeviation") || force) {
                streams->back()->setFmDeviation(settings.m_streams[istream].m_fmDeviation);
            };
            if (streamSettingsKeys.contains("dcFactor") || force) {
                streams->back()->setDcFactor(settings.m_streams[istream].m_dcFactor);
            };
            if (streamSettingsKeys.contains("iFactor") || force) {
                streams->back()->setIFactor(settings.m_streams[istream].m_iFactor);
            };
            if (streamSettingsKeys.contains("qFactor") || force) {
                streams->back()->setQFactor(settings.m_streams[istream].m_qFactor);
            };
            if (streamSettingsKeys.contains("phaseImbalance") || force) {
                streams->back()->setPhaseImbalance(settings.m_streams[istream].m_phaseImbalance);
            };
        }
    }

    if (deviceSettingsKeys.m_commonSettingsKeys.contains("fileRecordName") || force) {
        swgTestMISettings->setFileRecordName(new QString(settings.m_fileRecordName));
    }

    QString channelSettingsURL = QString("http://%1:%2/sdrangel/deviceset/%3/device/settings")
            .arg(settings.m_reverseAPIAddress)
            .arg(settings.m_reverseAPIPort)
            .arg(settings.m_reverseAPIDeviceIndex);
    m_networkRequest.setUrl(QUrl(channelSettingsURL));
    m_networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QBuffer *buffer=new QBuffer();
    buffer->open((QBuffer::ReadWrite));
    buffer->write(swgDeviceSettings->asJson().toUtf8());
    buffer->seek(0);

    // Always use PATCH to avoid passing reverse API settings
    m_networkManager->sendCustomRequest(m_networkRequest, "PATCH", buffer);

    delete swgDeviceSettings;
}

void TestMI::webapiReverseSendStartStop(bool start)
{
    SWGSDRangel::SWGDeviceSettings *swgDeviceSettings = new SWGSDRangel::SWGDeviceSettings();
    swgDeviceSettings->setDirection(0); // single Rx
    swgDeviceSettings->setOriginatorIndex(m_deviceAPI->getDeviceSetIndex());
    swgDeviceSettings->setDeviceHwType(new QString("TestSource"));

    QString channelSettingsURL = QString("http://%1:%2/sdrangel/deviceset/%3/device/run")
            .arg(m_settings.m_reverseAPIAddress)
            .arg(m_settings.m_reverseAPIPort)
            .arg(m_settings.m_reverseAPIDeviceIndex);
    m_networkRequest.setUrl(QUrl(channelSettingsURL));
    m_networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QBuffer *buffer=new QBuffer();
    buffer->open((QBuffer::ReadWrite));
    buffer->write(swgDeviceSettings->asJson().toUtf8());
    buffer->seek(0);

    if (start) {
        m_networkManager->sendCustomRequest(m_networkRequest, "POST", buffer);
    } else {
        m_networkManager->sendCustomRequest(m_networkRequest, "DELETE", buffer);
    }

    delete swgDeviceSettings;
}

void TestMI::networkManagerFinished(QNetworkReply *reply)
{
    QNetworkReply::NetworkError replyError = reply->error();

    if (replyError)
    {
        qWarning() << "TestMI::networkManagerFinished:"
                << " error(" << (int) replyError
                << "): " << replyError
                << ": " << reply->errorString();
        return;
    }

    QString answer = reply->readAll();
    answer.chop(1); // remove last \n
    qDebug("TestMI::networkManagerFinished: reply:\n%s", answer.toStdString().c_str());
}

bool TestMI::isRecording(unsigned int istream) const
{
    if (istream < m_fileSinks.size()) {
        return m_fileSinks[istream]->isRecording();
    } else {
        return false;
    }
}