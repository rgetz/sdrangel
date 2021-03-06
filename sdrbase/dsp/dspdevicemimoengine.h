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

#ifndef SDRBASE_DSP_DSPDEVICEMIMOENGINE_H_
#define SDRBASE_DSP_DSPDEVICEMIMOENGINE_H_

#include <QThread>

#include "dsp/dsptypes.h"
#include "util/message.h"
#include "util/messagequeue.h"
#include "util/syncmessenger.h"
#include "util/movingaverage.h"
#include "export.h"

class DeviceSampleMIMO;
class ThreadedBasebandSampleSource;
class ThreadedBasebandSampleSink;
class BasebandSampleSink;

class SDRBASE_API DSPDeviceMIMOEngine : public QThread {
	Q_OBJECT

public:
    class SetSampleMIMO : public Message {
        MESSAGE_CLASS_DECLARATION
    public:
        SetSampleMIMO(DeviceSampleMIMO* sampleMIMO) : Message(), m_sampleMIMO(sampleMIMO) { }
        DeviceSampleMIMO* getSampleMIMO() const { return m_sampleMIMO; }
    private:
        DeviceSampleMIMO* m_sampleMIMO;
    };

    class AddThreadedBasebandSampleSource : public Message {
        MESSAGE_CLASS_DECLARATION
    public:
        AddThreadedBasebandSampleSource(ThreadedBasebandSampleSource* threadedSampleSource, unsigned int index) :
            Message(),
            m_threadedSampleSource(threadedSampleSource),
            m_index(index)
        { }
        ThreadedBasebandSampleSource* getThreadedSampleSource() const { return m_threadedSampleSource; }
        unsigned int getIndex() const { return m_index; }
    private:
        ThreadedBasebandSampleSource* m_threadedSampleSource;
        unsigned int m_index;
    };

    class AddSourceStream : public Message {
        MESSAGE_CLASS_DECLARATION
    public:
        AddSourceStream(bool connect) : Message(), m_connect(connect) { }
        bool getConnect() const { return m_connect; }
    private:
        bool m_connect;
    };

    class RemoveLastSourceStream : public Message {
        MESSAGE_CLASS_DECLARATION
    };

    class AddSinkStream : public Message {
        MESSAGE_CLASS_DECLARATION
    public:
        AddSinkStream(bool connect) : Message(), m_connect(connect) { }
        bool getConnect() const { return m_connect; }
    private:
        bool m_connect;
    };

    class RemoveLastSinkStream : public Message {
        MESSAGE_CLASS_DECLARATION
    };

    class RemoveThreadedBasebandSampleSource : public Message {
	    MESSAGE_CLASS_DECLARATION

    public:
        RemoveThreadedBasebandSampleSource(ThreadedBasebandSampleSource* threadedSampleSource, unsigned int index) :
            Message(),
            m_threadedSampleSource(threadedSampleSource),
            m_index(index)
        { }
        ThreadedBasebandSampleSource* getThreadedSampleSource() const { return m_threadedSampleSource; }
        unsigned int getIndex() const { return m_index; }
    private:
        ThreadedBasebandSampleSource* m_threadedSampleSource;
        unsigned int m_index;
    };

    class AddThreadedBasebandSampleSink : public Message {
        MESSAGE_CLASS_DECLARATION
    public:
        AddThreadedBasebandSampleSink(ThreadedBasebandSampleSink* threadedSampleSink, unsigned int index) :
            Message(),
            m_threadedSampleSink(threadedSampleSink),
            m_index(index)
        { }
        ThreadedBasebandSampleSink* getThreadedSampleSink() const { return m_threadedSampleSink; }
        unsigned int getIndex() const { return m_index; }
    private:
        ThreadedBasebandSampleSink* m_threadedSampleSink;
        unsigned int m_index;
    };

    class RemoveThreadedBasebandSampleSink : public Message {
        MESSAGE_CLASS_DECLARATION
    public:
        RemoveThreadedBasebandSampleSink(ThreadedBasebandSampleSink* threadedSampleSink, unsigned int index) :
            Message(),
            m_threadedSampleSink(threadedSampleSink),
            m_index(index)
        { }
        ThreadedBasebandSampleSink* getThreadedSampleSink() const { return m_threadedSampleSink; }
        unsigned int getIndex() const { return m_index; }
    private:
        ThreadedBasebandSampleSink* m_threadedSampleSink;
        unsigned int m_index;
    };

    class AddBasebandSampleSink : public Message {
        MESSAGE_CLASS_DECLARATION
    public:
        AddBasebandSampleSink(BasebandSampleSink* sampleSink, unsigned int index) :
            Message(),
            m_sampleSink(sampleSink),
            m_index(index)
        { }
        BasebandSampleSink* getSampleSink() const { return m_sampleSink; }
        unsigned int getIndex() const { return m_index; }
    private:
        BasebandSampleSink* m_sampleSink;
        unsigned int m_index;
    };

    class RemoveBasebandSampleSink : public Message {
        MESSAGE_CLASS_DECLARATION
    public:
        RemoveBasebandSampleSink(BasebandSampleSink* sampleSink, unsigned int index) :
            Message(),
            m_sampleSink(sampleSink),
            m_index(index)
        { }
        BasebandSampleSink* getSampleSink() const { return m_sampleSink; }
        unsigned int getIndex() const { return m_index; }
    private:
        BasebandSampleSink* m_sampleSink;
        unsigned int m_index;
    };

    class AddSpectrumSink : public Message {
        MESSAGE_CLASS_DECLARATION
    public:
        AddSpectrumSink(BasebandSampleSink* sampleSink) : Message(), m_sampleSink(sampleSink) { }
        BasebandSampleSink* getSampleSink() const { return m_sampleSink; }
    private:
        BasebandSampleSink* m_sampleSink;
    };

    class RemoveSpectrumSink : public Message {
        MESSAGE_CLASS_DECLARATION
    public:
        RemoveSpectrumSink(BasebandSampleSink* sampleSink) : Message(), m_sampleSink(sampleSink) { }
        BasebandSampleSink* getSampleSink() const { return m_sampleSink; }
    private:
        BasebandSampleSink* m_sampleSink;
    };

    class GetErrorMessage : public Message {
        MESSAGE_CLASS_DECLARATION
    public:
        void setErrorMessage(const QString& text) { m_errorMessage = text; }
        const QString& getErrorMessage() const { return m_errorMessage; }
    private:
        QString m_errorMessage;
    };

    class GetMIMODeviceDescription : public Message {
        MESSAGE_CLASS_DECLARATION
    public:
        void setDeviceDescription(const QString& text) { m_deviceDescription = text; }
        const QString& getDeviceDescription() const { return m_deviceDescription; }
    private:
        QString m_deviceDescription;
    };

    class ConfigureCorrection : public Message {
        MESSAGE_CLASS_DECLARATION
    public:
        ConfigureCorrection(bool dcOffsetCorrection, bool iqImbalanceCorrection, unsigned int index) :
            Message(),
            m_dcOffsetCorrection(dcOffsetCorrection),
            m_iqImbalanceCorrection(iqImbalanceCorrection),
            m_index(index)
        { }
        bool getDCOffsetCorrection() const { return m_dcOffsetCorrection; }
        bool getIQImbalanceCorrection() const { return m_iqImbalanceCorrection; }
        unsigned int getIndex() const { return m_index; }
    private:
        bool m_dcOffsetCorrection;
        bool m_iqImbalanceCorrection;
        unsigned int m_index;
    };

    class SetSpectrumSinkInput : public Message {
        MESSAGE_CLASS_DECLARATION
    public:
        SetSpectrumSinkInput(bool sourceElseSink, int index) :
            m_sourceElseSink(sourceElseSink),
            m_index(index)
        { }
        bool getSourceElseSink() const { return m_sourceElseSink; }
        int getIndex() const { return m_index; }
    private:
        bool m_sourceElseSink;
        int m_index;
    };

	enum State {
		StNotStarted,  //!< engine is before initialization
		StIdle,        //!< engine is idle
		StReady,       //!< engine is ready to run
		StRunning,     //!< engine is running
		StError        //!< engine is in error
	};

	DSPDeviceMIMOEngine(uint32_t uid, QObject* parent = nullptr);
	~DSPDeviceMIMOEngine();

	MessageQueue* getInputMessageQueue() { return &m_inputMessageQueue; }

	void start(); //!< This thread start
	void stop();  //!< This thread stop

	bool initProcess();  //!< Initialize process sequence
	bool startProcess(); //!< Start process sequence
	void stopProcess();  //!< Stop process sequence

	void setMIMO(DeviceSampleMIMO* mimo); //!< Set the sample MIMO type
	DeviceSampleMIMO *getMIMO() { return m_deviceSampleMIMO; }
	void setMIMOSequence(int sequence); //!< Set the sample MIMO sequence in type
    uint getUID() const { return m_uid; }

    void addSourceStream(bool connect);
    void removeLastSourceStream();
    void addSinkStream(bool connect);
    void removeLastSinkStream();

	void addChannelSource(ThreadedBasebandSampleSource* source, int index = 0);    //!< Add a channel source that will run on its own thread
	void removeChannelSource(ThreadedBasebandSampleSource* source, int index = 0); //!< Remove a channel source that runs on its own thread
	void addChannelSink(ThreadedBasebandSampleSink* sink, int index = 0);          //!< Add a channel sink that will run on its own thread
	void removeChannelSink(ThreadedBasebandSampleSink* sink, int index = 0);       //!< Remove a channel sink that runs on its own thread

	void addAncillarySink(BasebandSampleSink* sink, int index = 0);    //!< Add an ancillary sink like a I/Q recorder
	void removeAncillarySink(BasebandSampleSink* sink, int index = 0); //!< Remove an ancillary sample sink

	void addSpectrumSink(BasebandSampleSink* spectrumSink);    //!< Add a spectrum vis baseband sample sink
	void removeSpectrumSink(BasebandSampleSink* spectrumSink); //!< Add a spectrum vis baseband sample sink
    void setSpectrumSinkInput(bool sourceElseSink, int index);

	State state() const { return m_state; } //!< Return DSP engine current state

	QString errorMessage(); //!< Return the current error message
	QString deviceDescription(); //!< Return the device description

   	void configureCorrections(bool dcOffsetCorrection, bool iqImbalanceCorrection, int isource); //!< Configure source DSP corrections

private:
    struct SourceCorrection
    {
        bool m_dcOffsetCorrection;
        bool m_iqImbalanceCorrection;
        double m_iOffset;
        double m_qOffset;
        int m_iRange;
        int m_qRange;
        int m_imbalance;
        MovingAverageUtil<int32_t, int64_t, 1024> m_iBeta;
        MovingAverageUtil<int32_t, int64_t, 1024> m_qBeta;
#if IMBALANCE_INT
        // Fixed point DC + IQ corrections
        MovingAverageUtil<int64_t, int64_t, 128> m_avgII;
        MovingAverageUtil<int64_t, int64_t, 128> m_avgIQ;
        MovingAverageUtil<int64_t, int64_t, 128> m_avgPhi;
        MovingAverageUtil<int64_t, int64_t, 128> m_avgII2;
        MovingAverageUtil<int64_t, int64_t, 128> m_avgQQ2;
        MovingAverageUtil<int64_t, int64_t, 128> m_avgAmp;
#else
        // Floating point DC + IQ corrections
        MovingAverageUtil<float, double, 128> m_avgII;
        MovingAverageUtil<float, double, 128> m_avgIQ;
        MovingAverageUtil<float, double, 128> m_avgII2;
        MovingAverageUtil<float, double, 128> m_avgQQ2;
        MovingAverageUtil<double, double, 128> m_avgPhi;
        MovingAverageUtil<double, double, 128> m_avgAmp;
#endif
        SourceCorrection()
        {
            m_dcOffsetCorrection = false;
            m_iqImbalanceCorrection = false;
            m_iOffset = 0;
            m_qOffset = 0;
            m_iRange = 1 << 16;
            m_qRange = 1 << 16;
            m_imbalance = 65536;
            m_iBeta.reset();
            m_qBeta.reset();
            m_avgAmp.reset();
            m_avgII.reset();
            m_avgII2.reset();
            m_avgIQ.reset();
            m_avgPhi.reset();
            m_avgQQ2.reset();
            m_iBeta.reset();
            m_qBeta.reset();
        }
    };

	uint32_t m_uid; //!< unique ID
    State m_state;

    QString m_errorMessage;
	QString m_deviceDescription;

	DeviceSampleMIMO* m_deviceSampleMIMO;
	int m_sampleMIMOSequence;

    MessageQueue m_inputMessageQueue;  //<! Input message queue. Post here.
	SyncMessenger m_syncMessenger;     //!< Used to process messages synchronously with the thread

	typedef std::list<BasebandSampleSink*> BasebandSampleSinks;
	std::vector<BasebandSampleSinks> m_basebandSampleSinks; //!< ancillary sample sinks on main thread (per input stream)

	typedef std::list<ThreadedBasebandSampleSink*> ThreadedBasebandSampleSinks;
	std::vector<ThreadedBasebandSampleSinks> m_threadedBasebandSampleSinks; //!< channel sample sinks on their own thread (per input stream)
    std::vector<int> m_sampleSinkConnectionIndexes;

	typedef std::list<ThreadedBasebandSampleSource*> ThreadedBasebandSampleSources;
	std::vector<ThreadedBasebandSampleSources> m_threadedBasebandSampleSources; //!< channel sample sources on their own threads (per output stream)
    std::vector<int> m_sampleSourceConnectionIndexes;

    std::vector<SourceCorrection> m_sourcesCorrections;

    BasebandSampleSink *m_spectrumSink; //!< The spectrum sink
    bool m_spectrumInputSourceElseSink; //!< Source else sink stream to be used as spectrum sink input
    unsigned int m_spectrumInputIndex;  //!< Index of the stream to be used as spectrum sink input

  	void run();
	void work(int nbWriteSamples); //!< transfer samples if in running state

	State gotoIdle();     //!< Go to the idle state
	State gotoInit();     //!< Go to the acquisition init state from idle
	State gotoRunning();  //!< Go to the running state from ready state
	State gotoError(const QString& errorMsg); //!< Go to an error state

    void handleSetMIMO(DeviceSampleMIMO* mimo); //!< Manage MIMO device setting
   	void iqCorrections(SampleVector::iterator begin, SampleVector::iterator end, int isource, bool imbalanceCorrection);

private slots:
	void handleData();                 //!< Handle data when samples have to be processed
    void workSampleSink(unsigned int sinkIndex);
	void handleSynchronousMessages();  //!< Handle synchronous messages with the thread
	void handleInputMessages();        //!< Handle input message queue
	void handleForwardToSpectrumSink(int nbSamples);
};

#endif // SDRBASE_DSP_DSPDEVICEMIMOENGINE_H_
