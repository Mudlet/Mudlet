let expect = require('chai').expect;

let jsonabc = require('./index');

describe('Trailing commas', function () {
  it('should remove from plain object', function () {
    expect(jsonabc.cleanJSON("{a: 'b', }")).to.be.equal("{a: 'b'}");
  });
  it('should remove from plain array', function () {
    expect(jsonabc.cleanJSON("[ 'b', ]")).to.be.equal("[ 'b']");
  });
});

describe('Sorting', function () {
  let input, inputPlainArrInPlainObj, outputPlainArrInPlainObj, expectedOutput, expectedOutputWithoutArray;

  beforeEach(function () {
    input = {
      'object': {
        'b': 2,
        'a': 1,
        'd': 4,
        'c': 3
      },
      'array': ['d', '1', 'c', 'a', 'b'],
      'collection': [{
        'b': 2,
        'a': 1,
        'd': 4,
        'c': 3
      }, {
        '__b1': 2,
        '__a2': 1,
        '__d3': 4,
        '__c4': 3
      },
      ['d', '1', 'c', 'a', 'b']
      ]
    };

    expectedOutput = {
      'array': [
        '1',
        'a',
        'b',
        'c',
        'd'
      ],
      'collection': [
        [
          '1',
          'a',
          'b',
          'c',
          'd'
        ],
        {
          '__a2': 1,
          '__b1': 2,
          '__c4': 3,
          '__d3': 4
        },
        {
          'a': 1,
          'b': 2,
          'c': 3,
          'd': 4
        }
      ],
      'object': {
        'a': 1,
        'b': 2,
        'c': 3,
        'd': 4
      }
    };

    expectedOutputWithoutArray = {
      'array': [
        'd',
        '1',
        'c',
        'a',
        'b'
      ],
      'collection': [{
        'a': 1,
        'b': 2,
        'c': 3,
        'd': 4
      },
      {
        '__a2': 1,
        '__b1': 2,
        '__c4': 3,
        '__d3': 4
      },
      [
        'd',
        '1',
        'c',
        'a',
        'b'
      ]
      ],
      'object': {
        'a': 1,
        'b': 2,
        'c': 3,
        'd': 4
      }
    };

    inputPlainArrInPlainObj = [{
      'b': 2,
      'a': 2
    },
    {
      'a': 1,
      'b': 1
    }
    ];

    outputPlainArrInPlainObj = [{
      'a': 1,
      'b': 1
    },

    {
      'a': 2,
      'b': 2
    }
    ];
  });

  it('should sort complex JSON', function () {
    expect(jsonabc.sortObj(input)).to.deep.equal(expectedOutput);
  });

  it('should sort JSON without array option', function () {
    expect(jsonabc.sortObj(input, true)).to.deep.equal(expectedOutputWithoutArray);
  });

  it('should sort plain objects in plain array', function () {
    expect(jsonabc.sortObj(inputPlainArrInPlainObj)).to.deep.equal(outputPlainArrInPlainObj);
  });
});
