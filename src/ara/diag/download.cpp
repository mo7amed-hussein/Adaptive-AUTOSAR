#include "./download.h"
#include "./routing/nrc_exception.h"

namespace ara
{
    namespace diag
    {
        const uint8_t DownloadService::cSid;
        const routing::TransferDirection DownloadService::cTransferDirection;

        DownloadService::DownloadService(
            const core::InstanceSpecifier &specifier,
            ReentrancyType reentrancy,
            routing::TransferData &transferData) : routing::RequestTransfer(specifier, reentrancy, cSid, transferData, cTransferDirection)
        {
        }

        std::future<OperationOutput> DownloadService::HandleMessage(
            const std::vector<uint8_t> &requestData,
            MetaInfo &metaInfo,
            CancellationHandler &&cancellationHandler)
        {
            std::promise<OperationOutput> _resultPromise;
            OperationOutput _response;

            uint8_t _nrc;
            uint8_t _dataFormatIdentifier;
            uint8_t _addressAndLengthFormatIdentifier;
            std::vector<uint8_t> _memoryAddressAndSize;

            bool _succeed{
                TryParseRequest(
                    requestData,
                    _dataFormatIdentifier,
                    _addressAndLengthFormatIdentifier,
                    _memoryAddressAndSize)};

            if (_succeed)
            {
                std::future<void> _resquestResultFuture{
                    RequestDownload(
                        _dataFormatIdentifier,
                        _addressAndLengthFormatIdentifier,
                        _memoryAddressAndSize,
                        metaInfo,
                        std::move(cancellationHandler))};

                try
                {
                    _resquestResultFuture.get();
                    _succeed = TryGeneratePositiveResponse(metaInfo, _response);

                    if (!_succeed)
                    {
                        _nrc = cUploadDownloadNotAccepted;
                    }
                }
                catch (const routing::NrcExecption &ex)
                {
                    _nrc = ex.GetNrc();
                }
            }
            else
            {
                _nrc = cIncorrectMessageLength;
            }

            if (!_succeed)
            {
                GenerateNegativeResponse(_response, cUploadDownloadNotAccepted);
            }

            _resultPromise.set_value(_response);
            std::future<OperationOutput> _result{_resultPromise.get_future()};
            return _result;
        }

        std::future<void> DownloadService::RequestDownload(
            uint8_t dataFormatIdentifier,
            uint8_t addressAndLengthFormatIdentifier,
            std::vector<uint8_t> memoryAddressAndSize,
            MetaInfo &metaInfo,
            CancellationHandler &&cancellationHandler)
        {
            std::promise<void> _resultPromise;

            uint8_t _nrc;
            size_t _memoryAddress;
            size_t _memorySize;

            bool _succeed{
                TryParseLengthFormat(
                    dataFormatIdentifier,
                    addressAndLengthFormatIdentifier,
                    memoryAddressAndSize,
                    _memoryAddress, _memorySize)};

            if (_succeed)
            {
                _succeed = TryRequestTransfer(_memoryAddress, _memorySize);

                if (!_succeed)
                {
                    _nrc = cUploadDownloadNotAccepted;
                }
            }
            else
            {
                _nrc = cRequestOutOfRange;
            }

            if (_succeed)
            {
                _resultPromise.set_value();
            }
            else
            {
                routing::NrcExecption _exception(_nrc);
                std::exception_ptr _exceptionPtr{std::make_exception_ptr(_exception)};
                _resultPromise.set_exception(_exceptionPtr);
            }

            std::future<void> _result{_resultPromise.get_future()};
            return _result;
        }
    }
}